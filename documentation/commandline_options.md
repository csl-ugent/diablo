# Command-line options
## Adding command-line options
Diablo provides with an easy way to specify options. The format to
specify an option is as follows:

~~~~
[group "name_of_group"] [hidden] [required|forbidden [ifset|ifnotset (option_list)]]option_type option_name
{
  [short="";]
  [long="";]
  [default=;]
  [description="description of the option"]
  [environment=""]
}
~~~~

Options can be grouped by giving them the same group name. If the hidden
flag is set, they will not be printed when diablo is invoked with the
help option. One can specify wheter the option is required and/or
forbidden as such or in the presence of other options. The option\_list
consist of options combined by `()`, `&&` and `||` in their usual
meaning. The different options supported by Diablo are:

~~~~
string_option
file_option
bool_option
int_option
path_option
count_option
usage_option
~~~~

These should be self-explaining. An example:

~~~~
usage_option help
{
  short="h";
  long="help";
  description="Prints this help message"
}
~~~~

The short string allows the option to be invoked as -h The long string
allows the option to be invoked as --help The description defines what
will be printed when diablo is invoked with the -h or --help option For
other options, a default value can be given. Note that the defaults for
a `bool_option` are `on` and `off`. Finally, an environment variable can
be specified which contains the value of the option.

## Generating the c and header file from the options file

Suppose we put the options in a file named `application.opt`. Then the c and
header file can be created as follows:

~~~~
opt_gen -d on -o application_cmdline -a application_options -l application_list -f Application application.opt
~~~~

To understand what this will do, here is (part of) the output of
opt\_gen -h

~~~~
Usage:
  [-d [on|off]]: Include diablosupport.h instead of opt_gen_handler.h
  [<file>]: The input file, defaults to stdin if not set
  [-r|--relative_path ]: Sets the relative path to util_options.h
  [-h|--help]: Generate this help message
  [-f <string>]: Sets the name of the functions.
  [-l <string>]: Sets the name of the option list.
  [-a <string>]: Sets the name of the array.
  [-o <string>]: Sets the output filename prefix. The files <string>.h and <string>.c are generated
~~~~

Now that we have generated the necessary files, how will this be used in
practice? For our example, this will typically be done as follows:

~~~~
/*The initialisation function that was auto-generated by opt_gen*/
ApplicationInit();

/*The commandline is parsed to get the values of the options that were specified at the commandline*/
OptionParseCommandLine(application_list,argc,argv,TRUE);
/*Options that were not specified at the commandline are searched for in the environment variables*/
OptionGetEnvironment(reachable_list);
/*Options that were not specified at the commandline or in the environment are set to their default values*/
OptionDefaults(reachable_list);

/*A function has been generated by opt_gen can be used to check is the requirements (required/forbidden) are met*/
ApplicationVerify();

/*The finalisation function that was auto-generated by opt_gen*/
ApplicationFini();
~~~~
