#include "PID.h"

#define _CONSTRAIN(amt, low, high)  ((amt) < (low) ? (low) : ((amt) > (high) ? (high) : (amt)))

void PID_Init(PIDController *pid, float P, float I, float D, float ramp, float limit)
{
    pid->P = P;
    pid->I = I;
    pid->D = D;
    pid->output_ramp = ramp;
    pid->limit = limit;
    pid->error_prev = 0.0f;
    pid->output_prev = 0.0f;
    pid->integral_prev = 0.0f;
}

void PID_Reset(PIDController *pid)
{
    pid->error_prev = 0.0f;
    pid->output_prev = 0.0f;
    pid->integral_prev = 0.0f;
}

float PID_Operator(PIDController *pid, float error, float Ts)
{
    float P, I, D, output, raw_output;

    P = pid->P * error;
    I = pid->integral_prev + pid->I * Ts * 0.5f * (error + pid->error_prev);
    I = _CONSTRAIN(I, -pid->limit, pid->limit);
    D = pid->D * (error - pid->error_prev) / Ts;

    raw_output = P + I + D;

    /* 输出饱和时停止积分，避免反向超调后的长时间追击 */
    if ((raw_output > pid->limit && error > 0.0f) ||
        (raw_output < -pid->limit && error < 0.0f))
    {
        I = pid->integral_prev;
        raw_output = P + I + D;
    }
    output = raw_output;

    /* 输出变化率限制 */
    if (pid->output_ramp > 0.0f)
    {
        float rate = (output - pid->output_prev) / Ts;
        if (rate > pid->output_ramp)
            output = pid->output_prev + pid->output_ramp * Ts;
        else if (rate < -pid->output_ramp)
            output = pid->output_prev - pid->output_ramp * Ts;
    }

    output = _CONSTRAIN(output, -pid->limit, pid->limit);

    pid->integral_prev = I;
    pid->output_prev = output;
    pid->error_prev = error;
    return output;
}
