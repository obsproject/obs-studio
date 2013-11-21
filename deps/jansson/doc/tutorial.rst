.. _tutorial:

********
Tutorial
********

.. highlight:: c

In this tutorial, we create a program that fetches the latest commits
of a repository in GitHub_ over the web. `GitHub API`_ uses JSON, so
the result can be parsed using Jansson.

To stick to the the scope of this tutorial, we will only cover the the
parts of the program related to handling JSON data. For the best user
experience, the full source code is available:
:download:`github_commits.c`. To compile it (on Unix-like systems with
gcc), use the following command::

    gcc -o github_commits github_commits.c -ljansson -lcurl

libcurl_ is used to communicate over the web, so it is required to
compile the program.

The command line syntax is::

    github_commits USER REPOSITORY

``USER`` is a GitHub user ID and ``REPOSITORY`` is the repository
name. Please note that the GitHub API is rate limited, so if you run
the program too many times within a short period of time, the sever
starts to respond with an error.

.. _GitHub: https://github.com/
.. _GitHub API: http://developer.github.com/
.. _libcurl: http://curl.haxx.se/


.. _tutorial-github-commits-api:

The GitHub Repo Commits API
===========================

The `GitHub Repo Commits API`_ is used by sending HTTP requests to
URLs like ``https://api.github.com/repos/USER/REPOSITORY/commits``,
where ``USER`` and ``REPOSITORY`` are the GitHub user ID and the name
of the repository whose commits are to be listed, respectively.

GitHub responds with a JSON array of the following form:

.. code-block:: none

    [
        {
            "sha": "<the commit ID>",
            "commit": {
                "message": "<the commit message>",
                <more fields, not important to this tutorial...>
            },
            <more fields...>
        },
        {
            "sha": "<the commit ID>",
            "commit": {
                "message": "<the commit message>",
                <more fields...>
            },
            <more fields...>
        },
        <more commits...>
    ]

In our program, the HTTP request is sent using the following
function::

    static char *request(const char *url);

It takes the URL as a parameter, preforms a HTTP GET request, and
returns a newly allocated string that contains the response body. If
the request fails, an error message is printed to stderr and the
return value is *NULL*. For full details, refer to :download:`the code
<github_commits.c>`, as the actual implementation is not important
here.

.. _GitHub Repo Commits API: http://developer.github.com/v3/repos/commits/

.. _tutorial-the-program:

The Program
===========

First the includes::

    #include <string.h>
    #include <jansson.h>

Like all the programs using Jansson, we need to include
:file:`jansson.h`.

The following definitions are used to build the GitHub API request
URL::

   #define URL_FORMAT   "https://api.github.com/repos/%s/%s/commits"
   #define URL_SIZE     256

The following function is used when formatting the result to find the
first newline in the commit message::

    /* Return the offset of the first newline in text or the length of
       text if there's no newline */
    static int newline_offset(const char *text)
    {
        const char *newline = strchr(text, '\n');
        if(!newline)
            return strlen(text);
        else
            return (int)(newline - text);
    }

The main function follows. In the beginning, we first declare a bunch
of variables and check the command line parameters::

    int main(int argc, char *argv[])
    {
        size_t i;
        char *text;
        char url[URL_SIZE];

        json_t *root;
        json_error_t error;

        if(argc != 3)
        {
            fprintf(stderr, "usage: %s USER REPOSITORY\n\n", argv[0]);
            fprintf(stderr, "List commits at USER's REPOSITORY.\n\n");
            return 2;
        }

Then we build the request URL using the user and repository names
given as command line parameters::

    snprintf(url, URL_SIZE, URL_FORMAT, argv[1], argv[2]);

This uses the ``URL_SIZE`` and ``URL_FORMAT`` constants defined above.
Now we're ready to actually request the JSON data over the web::

    text = request(url);
    if(!text)
        return 1;

If an error occurs, our function ``request`` prints the error and
returns *NULL*, so it's enough to just return 1 from the main
function.

Next we'll call :func:`json_loads()` to decode the JSON text we got
as a response::

    root = json_loads(text, 0, &error);
    free(text);

    if(!root)
    {
        fprintf(stderr, "error: on line %d: %s\n", error.line, error.text);
        return 1;
    }

We don't need the JSON text anymore, so we can free the ``text``
variable right after decoding it. If :func:`json_loads()` fails, it
returns *NULL* and sets error information to the :type:`json_error_t`
structure given as the second parameter. In this case, our program
prints the error information out and returns 1 from the main function.

Now we're ready to extract the data out of the decoded JSON response.
The structure of the response JSON was explained in section
:ref:`tutorial-github-commits-api`.

We check that the returned value really is an array::

    if(!json_is_array(root))
    {
        fprintf(stderr, "error: root is not an array\n");
        json_decref(root);
        return 1;
    }

Then we proceed to loop over all the commits in the array::

    for(i = 0; i < json_array_size(root); i++)
    {
        json_t *data, *sha, *commit, *message;
        const char *message_text;

        data = json_array_get(root, i);
        if(!json_is_object(data))
        {
            fprintf(stderr, "error: commit data %d is not an object\n", i + 1);
            json_decref(root);
            return 1;
        }
    ...

The function :func:`json_array_size()` returns the size of a JSON
array. First, we again declare some variables and then extract the
i'th element of the ``root`` array using :func:`json_array_get()`.
We also check that the resulting value is a JSON object.

Next we'll extract the commit ID (a hexadecimal SHA-1 sum),
intermediate commit info object, and the commit message from that
object. We also do proper type checks::

        sha = json_object_get(data, "sha");
        if(!json_is_string(sha))
        {
            fprintf(stderr, "error: commit %d: sha is not a string\n", i + 1);
            json_decref(root);
            return 1;
        }

        commit = json_object_get(data, "commit");
        if(!json_is_object(commit))
        {
            fprintf(stderr, "error: commit %d: commit is not an object\n", i + 1);
            json_decref(root);
            return 1;
        }

        message = json_object_get(commit, "message");
        if(!json_is_string(message))
        {
            fprintf(stderr, "error: commit %d: message is not a string\n", i + 1);
            json_decref(root);
            return 1;
        }
    ...

And finally, we'll print the first 8 characters of the commit ID and
the first line of the commit message. A C-style string is extracted
from a JSON string using :func:`json_string_value()`::

        message_text = json_string_value(message);
        printf("%.8s %.*s\n",
               json_string_value(id),
               newline_offset(message_text),
               message_text);
    }

After sending the HTTP request, we decoded the JSON text using
:func:`json_loads()`, remember? It returns a *new reference* to the
JSON value it decodes. When we're finished with the value, we'll need
to decrease the reference count using :func:`json_decref()`. This way
Jansson can release the resources::

    json_decref(root);
    return 0;

For a detailed explanation of reference counting in Jansson, see
:ref:`apiref-reference-count` in :ref:`apiref`.

The program's ready, let's test it and view the latest commits in
Jansson's repository::

    $ ./github_commits akheron jansson
    1581f26a Merge branch '2.3'
    aabfd493 load: Change buffer_pos to be a size_t
    bd72efbd load: Avoid unexpected behaviour in macro expansion
    e8fd3e30 Document and tweak json_load_callback()
    873eddaf Merge pull request #60 from rogerz/contrib
    bd2c0c73 Ignore the binary test_load_callback
    17a51a4b Merge branch '2.3'
    09c39adc Add json_load_callback to the list of exported symbols
    cbb80baf Merge pull request #57 from rogerz/contrib
    040bd7b0 Add json_load_callback()
    2637faa4 Make test stripping locale independent
    <...>


Conclusion
==========

In this tutorial, we implemented a program that fetches the latest
commits of a GitHub repository using the GitHub Repo Commits API.
Jansson was used to decode the JSON response and to extract the commit
data.

This tutorial only covered a small part of Jansson. For example, we
did not create or manipulate JSON values at all. Proceed to
:ref:`apiref` to explore all features of Jansson.
