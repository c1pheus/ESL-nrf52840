#include <stdint.h>

#include "sdk_config.h"
#include "app_error.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "nrf_log_backend_usb.h"
#include "log.h"

uint32_t log_init(void)
{
    ret_code_t err_code = NRF_LOG_INIT(NULL);

    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    NRF_LOG_DEFAULT_BACKENDS_INIT();
    return NRF_SUCCESS;
}

void log_process(void)
{
    (void)NRF_LOG_PROCESS();
    LOG_BACKEND_USB_PROCESS();
}
