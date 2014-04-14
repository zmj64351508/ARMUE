#include <stdio.h>
#include "_types.h"
#include "error_code.h"
#include "core_connect.h"

typedef HANDLE process_id_t;

process_id_t create_process()
{
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    ZeroMemory( &si, sizeof(si) );
    si.cb = sizeof(si);
    ZeroMemory( &pi, sizeof(pi) );

    if( !CreateProcess( NULL,   // No module name (use command line)
       "./ARMUE.exe -g -c",      // Command line
       NULL,           // Process handle not inheritable
       NULL,           // Thread handle not inheritable
       FALSE,          // Set handle inheritance to FALSE
       0,              // No creation flags
       NULL,           // Use parent's environment block
       NULL,           // Use parent's starting directory
       &si,            // Pointer to STARTUPINFO structure
       &pi )           // Pointer to PROCESS_INFORMATION structure
       )
    {
        LOG(LOG_ERROR, "CreateProcess failed %ld\n", GetLastError());
        return INVALID_HANDLE_VALUE;
    }else{
        return pi.hProcess;
    }
}

void wait_close_child_process(process_id_t pid)
{
    // Wait until child process exits.
    WaitForSingleObject( pid, INFINITE );

    // Close process and thread handles.
    CloseHandle( pid );
}



int main(int argc, char **argv)
{
    int retval;

//    process_id_t pid = create_process();
//    if(pid < 0){
//        return -1;
//    }

    core_connect_t *core_connect = create_core_connect(1024, "\\\\.\\Pipe\\armue");
    retval = connect_armue_core(core_connect);
    if(retval != SUCCESS){
        return -1;
    }
    // connected
    restart_send_packet(core_connect);
    make_monitor_data_packet(core_connect, PERI_I2C, 0, "abc", 3);
    make_monitor_data_packet(core_connect, PERI_I2C, 0, "ddd", 3);
    core_connect->send_buf[core_connect->send_len - 1] = 10;
    send_to_core(core_connect);
    while(1);
#if 0
    while(1){
        long int recv_len = recv_from_core(core_connect, FALSE);
        // do something related to received message
        // ...
        if(recv_len > 0){
            core_connect->recv_buf[recv_len] = '\0';
            printf("%s\n", core_connect->recv_buf);
        }
        // do something that change the send buffer of core_connnect

        core_connect->send_buf[0] = 'a';
        core_connect->send_len = 1;
        // check whether need to send something to core
        send_to_core(core_connect);
        Sleep(1);
    }
#endif

    //wait_close_child_process(pid);

    return 0;
}
