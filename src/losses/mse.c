#include "losses/mse.h"
#include "autograd/computational_graph/computational_graph_link.h"
#include <stdlib.h>
#include <stdio.h>

typedef enum mse_loss_operand
{
    MSE_PREDICTED,
    MSE_TARGET
} mse_loss_operand;

static void mse_loss_backpropagate_predicted(const struct backpropagation_context *const ctx, const struct tensor *const grad_wrt_out, struct tensor *grad_wrt_operand);
static void mse_loss_backpropagate_target(const struct backpropagation_context *const ctx, const struct tensor *const grad_wrt_out, struct tensor *grad_wrt_operand);

cgrad_error mse_loss(const struct tensor *const y_pred, const struct tensor *const y_target, struct tensor *const z)
{
    if (!y_pred || !y_target || !z)
    {
        return TENSOR_NULL;
    }
    if (!y_pred->data || !y_target->data || !z->data)
    {
        return TENSOR_DATA_NULL;
    }
    if (y_pred->data_size != y_target->data_size)
    {
        return TENSOR_DATA_SIZE_MISMATCH;
    }
    if (!tensor_same_shape(y_pred, y_target))
    {
        return TENSOR_SHAPE_MISMATCH;
    }

    double batch_size = y_pred->shape[0];
    z->data[0] = 0;

    for (size_t i = 0; i < batch_size; i++)
    {
        // Compute sample squared error and sum it
        double difference = y_pred->data[i] - y_target->data[i];
        z->data[0] += (0.5 * difference * difference);
    }
    z->data[0] /= batch_size;

    return NO_ERROR;
}

cgrad_error mse_loss_graph(struct tensor *const y_pred, struct tensor *const y_target, struct tensor *const z, struct autograd_allocators *ag_allocators)
{
    cgrad_error err = mse_loss(y_pred, y_target, z);
    if (err != NO_ERROR)
    {
        return err;
    }

    add_computational_graph_link(y_pred, MSE_PREDICTED, z, &mse_loss_backpropagate_predicted, ag_allocators);
    add_computational_graph_link(y_target, MSE_TARGET, z, &mse_loss_backpropagate_target, ag_allocators);

    return NO_ERROR;
}

static void mse_loss_backpropagate_predicted(const struct backpropagation_context *const ctx, const struct tensor* const grad_wrt_out, struct tensor* grad_wrt_operand)
{
    const struct tensor *predicted = ctx->operands[MSE_PREDICTED];
    const struct tensor *target= ctx->operands[MSE_TARGET];
    double batch_size = target->shape[0];
    for (size_t i = 0; i < batch_size; i++)
    {
        grad_wrt_operand->data[i] = (predicted->data[i] - target->data[i]) / batch_size;
    }
}

static void mse_loss_backpropagate_target(const struct backpropagation_context *const ctx, const struct tensor* const grad_wrt_out, struct tensor* grad_wrt_operand)
{
    mse_loss_backpropagate_predicted(ctx, grad_wrt_out, grad_wrt_operand);

    // Gradient is the same but mult by -1
    for (size_t i = 0; i < grad_wrt_operand->shape[0]; i++)
    {
        grad_wrt_operand->data[i] *= -1;
    }
}