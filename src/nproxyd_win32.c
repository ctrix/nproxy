
#include <winsock2.h>
#include <windows.h>

#define snprintf sprintf_s
#define SERVICENAME_DEFAULT "NProxy"
#define SERVICENAME_MAXLEN 256
static char service_name[SERVICENAME_MAXLEN];

/* event to signal shutdown (for you unix people, this is like a pthread_cond) */
static HANDLE shutdown_event;

/* we need these vars to handle the service */
SERVICE_STATUS_HANDLE hStatus;
SERVICE_STATUS status;

/* Handler function for service start/stop from the service */
void WINAPI ServiceCtrlHandler(DWORD control) {
    switch (control) {
        case SERVICE_CONTROL_SHUTDOWN:
        case SERVICE_CONTROL_STOP:
            /* Shutdown */
            running = 0;

            /* set service status values */
            status.dwCurrentState = SERVICE_STOPPED;
            status.dwWin32ExitCode = 0;
            status.dwCheckPoint = 0;
            status.dwWaitHint = 0;
            break;
        case SERVICE_CONTROL_INTERROGATE:
            /* we already set the service status every time it changes. */
            /* if there are other times we change it and don't update, we should do so here */
            break;
    }

    SetServiceStatus(hStatus, &status);
}

/* the main service entry point */
void WINAPI service_main(DWORD numArgs, char **args) {
    const char *err = NULL;     /* error value for return from initialization */

    /*  we have to initialize the service-specific stuff */
    memset(&status, 0, sizeof(SERVICE_STATUS));

    status.dwServiceType = SERVICE_WIN32;
    status.dwCurrentState = SERVICE_START_PENDING;
    status.dwControlsAccepted = SERVICE_ACCEPT_STOP;

    /* register our handler for service control messages */
    hStatus = RegisterServiceCtrlHandler(service_name, &ServiceCtrlHandler);

    /* update the service status */
    SetServiceStatus(hStatus, &status);

    /* attempt to initialize nproxy and load modules */
    if (nproxy_library_init(NULL) != APR_STATUS_SUCCESS) {
        /* NProxy did not start successfully */
        status.dwCurrentState = SERVICE_STOPPED;
    } else {
        /* NProxy started */
        status.dwCurrentState = SERVICE_RUNNING;
    }

    /* update the service status */
    SetServiceStatus(hStatus, &status);
}

static apr_status_t nproxy_win32_service_start(void) {
    /* New installs will always have the service name specified, but keep a default for compat */

    /* Attempt to start service */
    SERVICE_TABLE_ENTRY dispatchTable[] = {
        {service_name, &service_main},
        {NULL, NULL}
    };

    if (StartServiceCtrlDispatcher(dispatchTable) == 0) {
        /* Not loaded as a service */
        fprintf(stderr, "Error NProxy loaded as a console app with -service option\n");
        fprintf(stderr, "To install the service load NProxy with -install\n");
    }
    exit(0);
}

static apr_status_t nproxy_win32_service_install(void) {
    char exePath[1024];
    char servicePath[1024];

    GetModuleFileName(NULL, exePath, 1024);

    snprintf(servicePath, sizeof(servicePath), "%s -service %s", exePath, service_name);

    {
        /* Perform service installation */
        SC_HANDLE hService;
        SC_HANDLE hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
        if (!hSCManager) {
            fprintf(stderr, "Could not open service manager (%d).\n", GetLastError());
            exit(1);
        }

        hService = CreateService(hSCManager, service_name, service_name, GENERIC_READ | GENERIC_EXECUTE | SERVICE_CHANGE_CONFIG, SERVICE_WIN32_OWN_PROCESS, SERVICE_AUTO_START, SERVICE_ERROR_IGNORE, servicePath, NULL, NULL, NULL, NULL,  /* Service start name */
                                 NULL);

        if (!hService) {
            fprintf(stderr, "Error creating NProxy service (%d).\n", GetLastError());
        } else {
            /* Set desc, and don't care if it succeeds */
            SERVICE_DESCRIPTION desc;
            desc.lpDescription = "The NProxy service.";
            if (!ChangeServiceConfig2(hService, SERVICE_CONFIG_DESCRIPTION, &desc)) {
                fprintf(stderr, "NProxy installed, but could not set the service description (%d).\n", GetLastError());
            }
            CloseServiceHandle(hService);
        }

        CloseServiceHandle(hSCManager);
        exit(0);
    }
}

static apr_status_t nproxy_win32_service_uninstall(void) {

    /* Do the uninstallation */
    SC_HANDLE hService;
    SC_HANDLE hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);

    if (!hSCManager) {
        fprintf(stderr, "Could not open service manager (%d).\n", GetLastError());
        exit(1);
    }

    hService = OpenService(hSCManager, service_name, DELETE);

    if (hService != NULL) {
        /* remove the service! */
        if (!DeleteService(hService)) {
            fprintf(stderr, "Error deleting service (%d).\n", GetLastError());
        }
        CloseServiceHandle(hService);
    } else {
        fprintf(stderr, "Error opening service (%d).\n", GetLastError());
    }

    CloseServiceHandle(hSCManager);
    exit(0);
}

#define KEYS_REGISTRY_POSITION "Software\\Ctrix\\NProxy"

static apr_status_t LoadRegValue(char *name, char *value, size_t vlen) {
    HKEY hkey;
    DWORD dwDisposition;
    DWORD dwType, dwSize;

    if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, TEXT(KEYS_REGISTRY_POSITION), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_READ, NULL, &hkey, &dwDisposition) == ERROR_SUCCESS) {
        /*
           dwType = REG_DWORD;
           dwSize = sizeof(DWORD);

           RegQueryValueEx(
           hkey, TEXT("MaxFileSize"), NULL, &dwType,
           (PBYTE)&m_dwMaxFileSize, &dwSize
           );
         */

        dwType = REG_EXPAND_SZ;
        dwSize = vlen;
        RegQueryValueEx(hkey, TEXT(name), NULL, &dwType, (LPBYTE) value, &dwSize);

        RegCloseKey(hkey);
        return APR_STATUS_SUCCESS;
    }
    return APR_STATUS_ERROR;
}

static apr_status_t SaveRegValue(char *name, char *value) {
    HKEY hkey;
    DWORD dwDisposition;

    DWORD dwType, dwSize;

    if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, TEXT(KEYS_REGISTRY_POSITION), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hkey, &dwDisposition) == ERROR_SUCCESS) {
        /*
           dwType = REG_DWORD;
           dwSize = sizeof(DWORD);
           RegSetValueEx(
           hkey, TEXT("MaxFileSize"), 0, dwType,
           (PBYTE)&m_dwMaxFileSize, dwSize);
         */

        dwType = REG_EXPAND_SZ;
        dwSize = (lstrlen(value) + 1) * sizeof(TCHAR);
        RegSetValueEx(hkey, TEXT(name), 0, dwType, (LPBYTE) value, dwSize);

        RegCloseKey(hkey);
/*
	{
		char test[1024] = "bah";
		LoadRegValue(name, test, sizeof(test) );
		nn_log(NN_LOG_DEBUG, "---> %s \n\n", test);
		assert(0);
	}
*/
        return APR_STATUS_SUCCESS;
    }

    return APR_STATUS_ERROR;
}
