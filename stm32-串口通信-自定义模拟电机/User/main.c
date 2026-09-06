#include "system.h"
#include "usart.h"
#include "time.h"

int main(void)
{
    USART1_Init(9600);

    TIM4_Init(99, 7199);

    while(1)
    {
        if(command_ready)
        {
            command_ready = 0;

            parse_command();
        }
    }
}