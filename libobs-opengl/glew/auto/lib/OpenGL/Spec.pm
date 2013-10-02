package OpenGL::Spec;

# A very simple task further complicated by the fact that some people
# can't read, others use legacy Operating Systems, and others don't give
# a damn about using a halfway decent text editor.
#
# The code to parse the _template_ is so simple and straightforward...
# yet the code to parse the real spec files is this mess.

my %typemap = (
    bitfield    => "GLbitfield",
    boolean     => "GLboolean",
    # fsck up in EXT_vertex_array
    Boolean     => "GLboolean",
    byte        => "GLbyte",
    clampd      => "GLclampd",
    clampf      => "GLclampf",
    double      => "GLdouble",
    enum        => "GLenum",
    # Intel fsck up
    Glenum      => "GLenum",
    float       => "GLfloat",
    half        => "GLuint",
    int         => "GLint",
    short       => "GLshort",
    sizei       => "GLsizei",
    ubyte       => "GLubyte",
    uint        => "GLuint",
    ushort      => "GLushort",
    DMbuffer    => "void *",

    # ARB VBO introduces these
    sizeiptrARB => "GLsizeiptrARB",
    intptrARB   => "GLintptrARB",

    # ARB shader objects introduces these, charARB is at least 8 bits,
    # handleARB is at least 32 bits
    charARB     => "GLcharARB",
    handleARB   => "GLhandleARB",

    # GLX 1.3 defines new types which might not be available at compile time
    #GLXFBConfig   => "void*",
    #GLXFBConfigID => "XID",
    #GLXContextID  => "XID",
    #GLXWindow     => "XID",
    #GLXPbuffer    => "XID",

    # Weird stuff for some SGIX extension
    #GLXFBConfigSGIX   => "void*",
    #GLXFBConfigIDSGIX => "XID",
);

my %void_typemap = (
    void    => "GLvoid",
);

my $section_re  = qr{^[A-Z]};
my $function_re = qr{^(.+) ([a-z][a-z0-9_]*) \((.+)\)$}i;
my $token_re    = qr{^([A-Z0-9][A-Z0-9_]*):?\s+((?:0x)?[0-9A-F]+)(.*)$};
my $prefix_re   = qr{^(?:AGL | GLX | WGL)_}x;
my $eofnc_re    = qr{ \);?$ | ^$ }x;
my $function_re = qr{^(.+) ([a-z][a-z0-9_]*) \((.+)\)$}i;
my $prefix_re   = qr{^(?:gl | agl | wgl | glX)}x;
my $types_re    = __compile_wordlist_cap(keys %typemap);
my $voidtype_re = __compile_wordlist_cap(keys %void_typemap);

sub new($)
{
    my $class = shift;
    my $self = { section => {} };
    $self->{filename} = shift;
    local $/;
    open(my $fh, "<$self->{filename}") or die "Can't open $self->{filename}";
    my $content = <$fh>;
    my $section;
    my $s = $self->{section};

    $content =~ s{[ \t]+$}{}mg;
    # Join lines that end with a word-character and ones that *begin*
    # with one
    $content =~ s{(\w)\n(\w)}{$1 $2}sg;

    foreach (split /\n/, $content)
    {
        if (/$section_re/)
        {
            chomp;
            s/^Name String$/Name Strings/; # Fix common mistake
            $section = $_;
            $s->{$section} = "";
        }
        elsif (defined $section and exists $s->{$section})
        {
            s{^\s+}{}mg; # Remove leading whitespace
            $s->{$section} .= $_ . "\n";
        }
    }

    $s->{$_} =~ s{(?:^\n+|\n+$)}{}s foreach keys %$s;

    bless $self, $class;
}

sub sections()
{
    my $self = shift;
    keys %{$self->{section}};
}

sub name()
{
    my $self = shift;
    $self->{section}->{Name};
}

sub name_strings()
{
    my $self = shift;
    split("\n", $self->{section}->{"Name Strings"});
}

sub tokens()
{
    my $self = shift;
    my %tokens = ();
    foreach (split /\n/, $self->{section}->{"New Tokens"})
    {
        next unless /$token_re/;
        my ($name, $value) = ($1, $2);
        $name =~ s{^}{GL_} unless $name =~ /$prefix_re/;
        $tokens{$name} = $value;
    }

    return %tokens;
}

sub functions()
{
    my $self = shift;
    my %functions = ();
    my @fnc = ();

    foreach (split /\n/, $self->{section}->{"New Procedures and Functions"})
    {
        push @fnc, $_ unless ($_ eq "" or $_ eq "None");

        next unless /$eofnc_re/;

        if (__normalize_proto(@fnc) =~ /$function_re/)
        {
            my ($return, $name, $parms) = ($1, $2, $3);
            if (!__ignore_function($name, $extname))
            {
                $name =~ s/^/gl/ unless $name =~ /$prefix_re/;
                if ($name =~ /^gl/ && $name !~ /^glX/)
                {
                    $return =~ s/$types_re/$typemap{$1}/g;
                    $return =~ s/$voidtype_re/$void_typemap{$1}/g;
                    $parms  =~ s/$types_re/$typemap{$1}/g;
                    $parms  =~ s/$voidtype_re/$void_typemap{$1}/g;
                }
                $functions{$name} = {
                    rtype => $return,
                    parms => $parms,
                };
            }
        }
        @fnc = ();
    }

    return %functions;
}

sub __normalize_proto
{
    local $_ = join(" ", @_);
    s/\s+/ /g;                # multiple whitespace -> single space
    s/\s*\(\s*/ \(/;          # exactly one space before ( and none after
    s/\s*\)\s*/\)/;           # no after before or after )
    s/\s*\*([a-zA-Z])/\* $1/; # "* identifier" XXX: g missing?
    s/\*wgl/\* wgl/;          # "* wgl"        XXX: why doesn't the
    s/\*glX/\* glX/;          # "* glX"             previous re catch this?
    s/\.\.\./void/;           # ... -> void
    s/;$//;                   # remove ; at the end of the line
    return $_;
}

sub __ignore_function
{
    return 0;
}

sub __compile_regex
{
    my $regex = join('', @_);
    return qr/$regex/
}

sub __compile_wordlist_cap
{
    __compile_regex('\b(', join('|', @_), ')\b');
}
