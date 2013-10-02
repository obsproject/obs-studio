#!/usr/bin/perl
##
## Copyright (C) 2002-2008, Marcelo E. Magallon <mmagallo[]debian org>
## Copyright (C) 2002-2008, Milan Ikits <milan ikits[]ieee org>
##
## This program is distributed under the terms and conditions of the GNU
## General Public License Version 2 as published by the Free Software
## Foundation or, at your option, any later version.

use strict;
use warnings;

sub compile_regex
{
    my $regex = join('', @_);
    return qr/$regex/
}

my @sections = (
    "Name",
    "Name Strings?",
    "New Procedures and Functions",
    "New Tokens",
    "Additions to Chapter.*",
);

my %typemap = (
    bitfield => "GLbitfield",
    boolean  => "GLboolean",
    # fsck up in EXT_vertex_array
    Boolean  => "GLboolean",
    byte     => "GLbyte",
    clampd   => "GLclampd",
    clampf   => "GLclampf",
    double   => "GLdouble",
    enum     => "GLenum",
    # Intel fsck up
    Glenum   => "GLenum",
    float    => "GLfloat",
    half     => "GLhalf",
    int      => "GLint",
    short    => "GLshort",
    sizei    => "GLsizei",
    ubyte    => "GLubyte",
    uint     => "GLuint",
    ushort   => "GLushort",
    DMbuffer => "void *",
    # Nvidia video output fsck up
    int64EXT => "GLint64EXT",
    uint64EXT=> "GLuint64EXT",

    # ARB VBO introduces these.

    sizeiptr => "GLsizeiptr",
    intptr   => "GLintptr",
    sizeiptrARB => "GLsizeiptrARB",
    intptrARB   => "GLintptrARB",

    # ARB shader objects introduces these, charARB is at least 8 bits,
    # handleARB is at least 32 bits
    charARB => "GLcharARB",
    handleARB => "GLhandleARB",

    char => "GLchar",

    # OpenGL 3.2 and GL_ARB_sync

    int64  => "GLint64",
    uint64 => "GLuint64",
    sync   => "GLsync",

    # AMD_debug_output

    DEBUGPROCAMD => "GLDEBUGPROCAMD",

    # ARB_debug_output

    DEBUGPROCARB => "GLDEBUGPROCARB",

    # KHR_debug

    DEBUGPROC => "GLDEBUGPROC",

    vdpauSurfaceNV => "GLvdpauSurfaceNV",
    
    # GLX 1.3 defines new types which might not be available at compile time

    #GLXFBConfig   => "void*",
    #GLXFBConfigID => "XID",
    #GLXContextID  => "XID",
    #GLXWindow     => "XID",
    #GLXPbuffer    => "XID",

    # Weird stuff to some SGIX extension

    #GLXFBConfigSGIX   => "void*",
    #GLXFBConfigIDSGIX => "XID",

);

my %voidtypemap = (
    void    => "GLvoid",
);

my %taboo_tokens = (
    GL_ZERO => 1,
);

# list of function definitions to be ignored, unless they are being defined in
# the given spec.  This is an ugly hack arround the fact that people writing
# spec files seem to shut down all brain activity while they are at this task.
#
# This will be moved to its own file eventually.
#
# (mem, 2003-03-19)

my %fnc_ignore_list = (
    "BindProgramARB"                => "ARB_vertex_program",
    "ColorSubTableEXT"              => "EXT_color_subtable",
    "DeleteProgramsARB"             => "ARB_vertex_program",
    "GenProgramsARB"                => "ARB_vertex_program",
    "GetProgramEnvParameterdvARB"   => "ARB_vertex_program",
    "GetProgramEnvParameterfvARB"   => "ARB_vertex_program",
    "GetProgramLocalParameterdvARB" => "ARB_vertex_program",
    "GetProgramLocalParameterfvARB" => "ARB_vertex_program",
    "GetProgramStringARB"           => "ARB_vertex_program",
    "GetProgramivARB"               => "ARB_vertex_program",
    "IsProgramARB"                  => "ARB_vertex_program",
    "ProgramEnvParameter4dARB"      => "ARB_vertex_program",
    "ProgramEnvParameter4dvARB"     => "ARB_vertex_program",
    "ProgramEnvParameter4fARB"      => "ARB_vertex_program",
    "ProgramEnvParameter4fvARB"     => "ARB_vertex_program",
    "ProgramLocalParameter4dARB"    => "ARB_vertex_program",
    "ProgramLocalParameter4dvARB"   => "ARB_vertex_program",
    "ProgramLocalParameter4fARB"    => "ARB_vertex_program",
    "ProgramLocalParameter4fvARB"   => "ARB_vertex_program",
    "ProgramStringARB"              => "ARB_vertex_program",
    "glXCreateContextAttribsARB"    => "ARB_create_context_profile",
    "wglCreateContextAttribsARB"    => "WGL_ARB_create_context_profile",
);

my %regex = (
    eofnc    => qr/(?:\);?$|^$)/, # )$ | );$ | ^$
    extname  => qr/^[A-Z][A-Za-z0-9_]+$/,
    none     => qr/^\(none\)$/,
    function => qr/^(.+) ([a-z][a-z0-9_]*) \((.+)\)$/i,
    prefix   => qr/^(?:[aw]?gl|glX)/, # gl | agl | wgl | glX
    tprefix  => qr/^(?:[AW]?GL|GLX)_/, # GL_ | AGL_ | WGL_ | GLX_
    section  => compile_regex('^(', join('|', @sections), ')$'), # sections in spec
    token    => qr/^([A-Z0-9][A-Z0-9_x]*):?\s+((?:0x)?[0-9A-F]+)([^\?]*)$/, # define tokens
    types    => compile_regex('\b(', join('|', keys %typemap), ')\b'), # var types
    voidtype => compile_regex('\b(', keys %voidtypemap, ')\b '), # void type
);

# reshapes the the function declaration from multiline to single line form
sub normalize_prototype
{
    local $_ = join(" ", @_);
    s/\s+/ /g;                # multiple whitespace -> single space
    s/\<.*\>//g;              # remove <comments> from direct state access extension
    s/\<.*$//g;               # remove incomplete <comments> from direct state access extension
    s#/\*.*\*/##g;            # remove /* ... */ comments
    s/\s*\(\s*/ \(/;          # exactly one space before ( and none after
    s/\s*\)\s*/\)/;           # no space before or after )
    s/\s*\*([a-zA-Z])/\* $1/; # "* identifier"
    s/\*wgl/\* wgl/;          # "* wgl"
    s/\*glX/\* glX/;          # "* glX"
    s/\.\.\./void/;           # ... -> void
    s/;$//;                   # remove ; at the end of the line
    return $_;
}

# Ugly hack to work arround the fact that functions are declared in more
# than one spec file.
sub ignore_function($$)
{
    return exists($fnc_ignore_list{$_[0]}) && $fnc_ignore_list{$_[0]} ne $_[1]
}

sub parse_spec($)
{
    my $filename = shift;
    my $extname = "";
    my $vendortag = "";
    my @extnames = ();
    my %functions = ();
    my %tokens = ();

    my $section = "";
    my @fnc = ();

    my %proc = (
        "Name" => sub {
            if (/^([a-z0-9]+)_([a-z0-9_]+)/i)
            {
                $extname = "$1_$2";
                $vendortag = $1;
            }
        },

        "Name Strings" => sub {
            # Add extension name to extension list
        
           # Initially use $extname if (none) specified
            if (/$regex{none}/)
            {
                $_ = $extname;
            }

            if (/$regex{extname}/)
            {
                # prefix with "GL_" if prefix not present
                s/^/GL_/ unless /$regex{tprefix}/o;
                # Add extension name to extension list
                push @extnames, $_;
            }
        },

        "New Procedures and Functions" => sub {
            # if line matches end of function
            if (/$regex{eofnc}/)
            {
                # add line to function declaration
                push @fnc, $_;

                # if normalized version of function looks like a function
                if (normalize_prototype(@fnc) =~ /$regex{function}/)
                {
                    # get return type, name, and arguments from regex
                    my ($return, $name, $parms) = ($1, $2, $3);
                    if (!ignore_function($name, $extname))
                    {
                        # prefix with "gl" if prefix not present
                        $name =~ s/^/gl/ unless $name =~ /$regex{prefix}/;
                        # is this a pure GL function?
                        if ($name =~ /^gl/ && $name !~ /^glX/)
                        {
                            # apply typemaps
                            $return =~ s/$regex{types}/$typemap{$1}/og;
                            $return =~ s/void\*/GLvoid */og;
                            $parms =~ s/$regex{types}/$typemap{$1}/og;
                            $parms =~ s/$regex{voidtype}/$voidtypemap{$1}/og;
                            $parms =~ s/ void\* / GLvoid */og;
                        }
                        # add to functions hash
                        $functions{$name} = {
                            rtype => $return,
                            parms => $parms,
                        };
                    }
                }
                # reset function declaration
                @fnc = ();
            } elsif ($_ ne "" and $_ ne "None") {
                # if not eof, add line to function declaration
                push @fnc, $_
            }
        },

        "New Tokens" => sub {
            if (/$regex{token}/)
            {
                my ($name, $value) = ($1, $2);
                # prefix with "GL_" if prefix not present
                $name =~ s/^/GL_/ unless $name =~ /$regex{tprefix}/;
                # Add (name, value) pair to tokens hash, unless it's taboo
                $tokens{$name} = $value unless exists $taboo_tokens{$name};
            }
        },
    );

    # Some people can't read, the template clearly says "Name String_s_"
    $proc{"Name String"} = $proc{"Name Strings"};

    # Open spec file
    open SPEC, "<$filename" or return;

    # For each line of SPEC
    while(<SPEC>)
    {
        # Delete trailing newline character
        chomp;
        # Remove trailing white spaces
        s/\s+$//;
        # If starts with a capital letter, it must be a new section
        if (/^[A-Z]/)
        {
            # Match section name with one of the predefined names 
            $section = /$regex{section}/o ? $1 : "default";
        } else {
            # Line is internal to a section
            # Remove leading whitespace
            s/^\s+//;
            # Call appropriate section processing function if it exists
            &{$proc{$section}} if exists $proc{$section};
        }
    }

    close SPEC;

    return ($extname, \@extnames, \%tokens, \%functions);
}

#----------------------------------------------------------------------------------------

my @speclist = ();
my %extensions = ();

my $ext_dir = shift;
my $reg_http = "http://www.opengl.org/registry/specs/gl/";

# Take command line arguments or read list from file
if (@ARGV)
{
    @speclist = @ARGV;
} else {
    local $/; #???
    @speclist = split "\n", (<>);
}

foreach my $spec (sort @speclist)
{
    my ($extname, $extnames, $tokens, $functions) = parse_spec($spec);

    foreach my $ext (@{$extnames})
    {
        my $info = "$ext_dir/" . $ext;
        open EXT, ">$info";
        print EXT $ext . "\n";                       # Extension name
        my $specname = $spec;
        $specname =~ s/registry\/gl\/specs\///;
        print EXT $reg_http . $specname . "\n";      # Extension info URL
        print EXT $ext . "\n";                       # Extension string

        my $prefix = $ext;
        $prefix =~ s/^(.+?)(_.+)$/$1/;
        foreach my $token (sort { hex ${$tokens}{$a} <=> hex ${$tokens}{$b} } keys %{$tokens})
        {
            if ($token =~ /^$prefix\_.*/i)
            {
                print EXT "\t" . $token . " " . ${\%{$tokens}}{$token} . "\n";
            }
        }
        foreach my $function (sort keys %{$functions})
        {
            if ($function =~ /^$prefix.*/i)
            {
                print EXT "\t" . ${$functions}{$function}{rtype} . " " . $function . " (" . ${$functions}{$function}{parms} . ")" . "\n";
            }
        }
        close EXT;
    }
}
