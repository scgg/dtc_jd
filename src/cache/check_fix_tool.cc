/*
 * =====================================================================================
 *
 *       Filename:  check_fix_tool.cc
 *
 *    Description:  进行lru,hash的检测，并输出结果
 *
 *        Version:  1.0
 *        Created:  03/18/2009 12:02:36 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  frankyang (huanhuange), frankyang@tencent.com
 *        Company:  Tencent, China
 *
 * =====================================================================================
 */

#include <errno.h>
#include <string.h>

#include "check_logic.h"
#include "check_code.h"
#include "log.h"

int main (int argc, char** argv)
{
    /* init log */
    _init_log_ ("check_fix_");
    /* set log level */
    _set_log_level_ (6);

    int             ret_code    = CHECK_UNKNOW;
    CCheckLogic*    logic       = CCheckLogic::Instance ();

    if (NULL == logic)
    {
        log_error ("create check logic failed, error msg[%s]", strerror(errno));
        ret_code = CHECK_ERROR_LOGIC;
        goto exit;
    }

    ret_code = logic->open (argc, argv);
    if (CHECK_SUCC != ret_code)
    {
        goto exit;
    }

    ret_code = logic->do_check_logic ();
    if (CHECK_SUCC != ret_code)
    {
        goto exit;
    }

    ret_code = CHECK_SUCC;
    fprintf (stderr, "hash & lru is intact\n");

exit:

    if (NULL != logic)
    {
        logic->close ();
        CCheckLogic::Destroy ();
        logic = NULL;
    }

    if (CHECK_SUCC != ret_code && CHECK_ERROR_PARA != ret_code)
    {
        fprintf (stderr, "hash & lru is spoiled\n");
    }

    return ret_code;
}
