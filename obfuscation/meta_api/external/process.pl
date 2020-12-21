#!/usr/bin/env perl
use strict;
use warnings;
use Carp::Assert;

if (@ARGV < 1) {
  die "Usage: $0 <input C file>\n";
}

# open input file (C source)
my $fSource = $ARGV[0];
open my $hSource, '<', $fSource or die "Error: failed to open source file $fSource\n";

# current state
use constant {
  STATE_ROOT => 0,
  STATE_FUNCTION_SIGNATURE => 1,
  STATE_FUNCTION_BODY => 2,

  FUNCTION_NONE => 0,
  FUNCTION_SETTER => 1,
  FUNCTION_GETTER => 2,

  ARGUMENT_NONE => 0,
  ARGUMENT_INSTANCE => 1,
  ARGUMENT_META => 2,
};
my $state = STATE_ROOT;

# state information
my $function_type = FUNCTION_NONE;
my @function_effects = ();
my $function_returntype = "";
my $function_name = "";
my @function_arguments = ();
my @function_body = ();
my @function_variables = ();
my $function_line = 0;

sub generate_function() {
  print "// $function_returntype $function_name @ line $function_line\n";
  print "#define META_API_FUNCTION_NAME $function_name\n";

  # variables
  foreach my $variable (@function_variables) {
    print "META_API_DECLARE_VARIABLE($variable);\n";
  }

  # arguments
  my @argument_list = ();
  my $argument_uid = 0;
  foreach my $argument (@function_arguments) {
    my %x = %{$argument};

    if ($x{'arg_type'} == ARGUMENT_INSTANCE) {
      push (@argument_list, "META_API_DATASTRUCTURE(" . $x{'name'} . ')');
    }
    elsif ($x{'arg_type'} == ARGUMENT_META) {
      push (@argument_list, "META_API_USE_ARGUMENT(\"$argument_uid\", " . $x{'type'} . ', ' . $x{'name'} . ")");

      print "META_API_DECLARE_ARGUMENT(\"$argument_uid\", " . $x{'type'} . ", " . $x{'name'} . ', ' . $x{'annot_content'} . ");\n";
    }
    elsif ($x{'arg_type'} == ARGUMENT_NONE) {
      push (@argument_list, $x{'type'} . " " . $x{'name'});
    }
    else {
      die "unsupported argument type";
    }

    $argument_uid = $argument_uid + 1;
  }

  # signature
  if ($function_type == FUNCTION_SETTER) {
    print "META_API_SETTER";

    assert($#function_effects+1 > 0);
  }
  elsif ($function_type == FUNCTION_GETTER) {
    print "META_API_GETTER";

    assert($#function_effects+1 == 0);
    if ($function_returntype ne "int") {
      die "Error: getter must return type 'int'\n";
    }
  }
  else {
    die "Error: unsupported function type\n";
  }
  print "((" . join(", ", @argument_list) . "))\n";

  if ($function_type == FUNCTION_SETTER) {
    foreach my $effect (@function_effects) {
      print "META_API_SETTER_EFFECT($effect)\n";
    }
  }

  print "BODY({\n";
  # body lines
  foreach my $bodyline (@function_body) {
    print "  $bodyline\n";
  }
  print "})\n";

  print "#undef META_API_FUNCTION_NAME\n";

  $function_type = FUNCTION_NONE;
  @function_effects = ();
  $function_returntype = "";
  $function_name = "";
  @function_arguments = ();
  @function_body = ();
  @function_variables = ();
  $function_line = 0;
}

my $rIdent = qr/[a-zA-Z_][a-zA-Z_0-9]*/;
my $rAnnot = qr/\$($rIdent)(\(([^\)]*)\))?/;

print "#include \"diablo_meta_api.h\"\n";

my $line_nr = 0;
while (<$hSource>) {
  my $line = $_;
  chomp $line;
  $line_nr = $line_nr + 1;

  if ($line =~ m/^\/\//) {
    print "$line\n";
    next;
  }
  if ($line =~ m/^#/) {
    print "$line\n";
    next;
  }

  if ($state == STATE_ROOT) {
    if ($line =~ m/^\s*$rAnnot/) {
      # immediately print
      my $annot = $1;
      my $annot_content = defined $3 ? $3 : "";

      if ($annot eq "meta_var") {
        assert($annot_content ne "");
        print "META_API_DECLARE_GLOBAL_VARIABLE($annot_content);\n"
      }
      elsif ($annot eq "meta_constraint") {
        # skip for now
      }
      else {
        die "unsupported global annotation '$annot'";
      }
    }
    elsif ($line =~ m/^([^\(]+)\((.*?)\)$/) {
      $state = STATE_FUNCTION_SIGNATURE;

      $function_line = $line_nr;

      my $return_and_name = $1;
      my $arguments = $2;

      # separate return type and name
      $return_and_name =~ m/($rIdent)$/;
      $function_name = $1;
      $function_returntype = $return_and_name =~ s/$function_name$//r;
      $function_returntype =~ s/^\s+|\s+$//g;

      # extract arguments
      my @splitted_arglist = split(',', $arguments);
      my @real_args = ();
      my $add_to_previous = 0;
      foreach my $part (@splitted_arglist) {
        if ($add_to_previous) {
          my $last = pop @real_args;
          $last = join(',', $last, $part);
          push (@real_args, $last);

          if ($part =~ m/\)$/) {
            $add_to_previous = 0;
          }
        }
        else {
          push (@real_args, $part);

          if ($part =~ m/\$$rIdent\(/ && $part !~ m/\)$/) {
            $add_to_previous = 1;
          }
        }
      }

      # iterate through args
      foreach my $arg (@real_args) {
        $arg =~ s/^\s+|\s+$//g;
        # print "real arg: >$arg<\n";

        $arg =~ m/(($rIdent)($rAnnot)?)$/;
        my $arg_name_and_annot = $1;
        my $arg_name = $2;
        my $arg_annot = defined $4 ? $4 : "";
        my $arg_annot_content = defined $6 ? $6 : "";

        # 'arg_name_and_annot' may contain regex characters.
        # Disable the interpretation of those characters to replace the variable properly.
        my $arg_type = $arg =~ s/\Q$arg_name_and_annot\E//r;
        $arg_type =~ s/^\s+|\s+$//g;
        # print ">$arg_type< >$arg_name< >$arg_annot< >$arg_annot_content<\n";

        my %argument = (
          'type' => $arg_type,
          'name' => $arg_name,
          'annot_content' => $arg_annot_content
        );
        if ($arg_annot eq "meta_instance") {
          assert($arg_annot_content eq "");
          $argument{'arg_type'} = ARGUMENT_INSTANCE;
        }
        elsif ($arg_annot eq "meta_var") {
          $argument{'arg_type'} = ARGUMENT_META;
        }
        elsif ($arg_annot ne "") {
          die "unsupported argument type $arg\n";
        }
        else {
          $argument{'arg_type'} = ARGUMENT_NONE;
        }

        push (@function_arguments, \%argument);
      }
    }
    else {
      print "$line\n";
    }
  }
  elsif ($state == STATE_FUNCTION_SIGNATURE) {
    if ($line =~ m/^{$/) {
      $state = STATE_FUNCTION_BODY;
    }
  }
  elsif ($state == STATE_FUNCTION_BODY) {
    if ($line =~ m/^}$/) {
      $state = STATE_ROOT;

      generate_function();
    }
    else {
      # process one line of function body
      $line =~ m/^\s*$rAnnot/;

      my $annot_name = defined $1 ? $1 : "";
      my $annot_content = defined $3 ? $3 : "";

      if ($annot_name eq "meta_getter") {
        assert($annot_content eq "");
        $function_type = FUNCTION_GETTER;
      }
      elsif ($annot_name eq "meta_setter") {
        assert($annot_content ne "");
        push (@function_effects, $annot_content);
        $function_type = FUNCTION_SETTER;
      }
      elsif ($annot_name eq "meta_var") {
        assert($annot_content ne "");
        push (@function_variables, $annot_content);
      }
      else {
        # regular code: possibly substitute
        while ($line =~ m/\s+\$\$($rIdent)/) {
          my $varname = $1;
          $line =~ s/\s+\$\$[a-zA-Z_][a-zA-Z_0-9]*/ META_API_USE_GLOBAL_VARIABLE($varname)/;
        }
        while ($line =~ m/\s+\$($rIdent)/) {
          my $varname = $1;
          $line =~ s/\s+\$[a-zA-Z_][a-zA-Z_0-9]*/ META_API_USE_VARIABLE($varname)/;
        }
        push (@function_body, $line . "// line: $line_nr");
      }
    }
  }
}