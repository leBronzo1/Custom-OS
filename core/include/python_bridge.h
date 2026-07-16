#ifndef PYTHON_BRIDGE_H
#define PYTHON_BRIDGE_H

// Initiates our python functions to be run in c code
int python_bridge_init(void);

// Ends our ability to run python functions
void python_bridge_shutdown(void);

// Calls the python function get_system_info
char *python_bridge_get_system_info(void);

#endif