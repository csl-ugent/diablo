# The Function Broker

The Function Broker mechanism enables the usage of different
implementations to perform a function depending on the backend,
application, etc., while maintaining a single, stable core fit for all
the different backends and applications. The Broker interface consists
of three functions:

~~~~
void DiabloBrokerCallInstall(char * name, char * prototype, void * implementation, t_bool final, ...);
~~~~

This function can be used to install an implementation for the function
with name `name`. The boolean argument indicates whether or not this
implementation can be extended by other applications.

~~~~
t_bool DiabloBrokerCallExists(char * name);
~~~~
     

This function can be used to check is an implementation has been
installed for function `name`

~~~~
t_bool DiabloBrokerCall(char * name, ...);
~~~~

This function is used to execute the installed implementation(s) of the
function `name`. If no implementations are installed, nothing happens
and FALSE is returned. If one or more implementations have been
installed, they are executed in the order in which they have been
installed and TRUE is returned.
