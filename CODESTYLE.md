OBS Studio Code Style Guidelines
================================

The project requires all contributions to have their source code formatted using appropriate formatting tools to reduce the potential impact of stylistic changes to a structured “diff” view of code.

In addition to those automatically enforceable stylistic choices the project also prefers contributions to follow a set of architectural guidelines that are chosen to reduce undefined or unexpected behavior and make code easier to reason about during reviews or maintenance.

## Reducing Potential For Errors

The following guidelines have been established as good practices to reduce some common potential for errors or naive coding practices that have lead to such errors in the past:

* **Always Initialize Variables** - Depending on the programming language, language standard, compiler, and platform, rules for when and how variables are initialized automatically might differ. Thus some variables might have random values at program start and naive code might interpret any value but “0” to mean “correctly initialized” and run into unexpected behavior.
    * Do not declare or initialize multiple variables on the same line, do not mix declarations and initializations

```C
int i, v = 0;    // BAD - v is initialized to 0, i is uninitialized


int i = 0;       // GOOD - i is explicitly declared and initialized
int v = 0;       // GOOD - v is separately declared and initialized
```

* **Do not use “0” as a valid enumeration value by default** - in many cases an enum requires an explicit choice to be made (only one value of a set of “valid” values can be set) and “not making a choice” should not be considered valid

```C
enum state { ACTIVE,           // BAD - zero-initialized enum potentially
             INACTIVE,         // leads to implicit state changes.
             DELETED
};


enum state { INVALID,          // GOOD - zero-initialized enum produces
             ACTIVE,           // an invalid value by default, avoiding
             INACTIVE,         // an implicit state change.
             DELETED
};


enum state { ACTIVE = 1,       // GOOD, zero-initialization fails because
             INACTIVE = 2,     // it’s not a valid enum value to begin with.
             DELETED = 3
};
```

* **Use natural language for variables and types** - expressive code is easier to reason about for maintainers and reviewers and also reduces the mental burden when returning to the same code after even a short absence, as code “says what it does” and variables “tell what they represent”.

```C
int c = 1;                  // BAD: What does “c” represent?
int count = 2;              // BAD: Count of “what”?
int num_bytes = 3;          // GOOD: “Num(ber) of bytes”


bool valid;                 // BAD: Meaning is ambiguous
bool has_valid_key;         // GOOD: Describes state of element in an object
bool is_valid;              // GOOD: Describes state of element itself
bool did_send_packet;       // GOOD: Describes state of transaction.


float dur = 1.0;             // BAD: Unit of duration unknown
float duration_ms = 5.0;     // GOOD: Unit of duration encoded within name


// GOOD: Function signature communicates the unit of the delay explicitly.
start_transition_with_delay(transition_type *transition, float delay_ms);
```

* **Use compound types rather than individual variables** - information like the size or dimension of an object, a timescale, a position in space, should be logically encoded in a compound type and only “unwrapped” when consumed

```C
// GOOD: Function signature communicates the unit of the delay explicitly.
start_transition_with_delay(transition_type *transition, float delay_ms);


// EVEN BETTER: Time values encoded as pieces of a fraction (1/1000th of a
//              second representing a “microsecond”) and explicitly passed to
//              the function.
typedef struct {
    int_64_t time_value;
    int_32_t time_scale;
} time_unit;


start_transition_with_delay(transition_type *transition, time_unit delay);


// BAD: Behavior encoded in unrelated booleans, maybe even conflicting
start_transition(transition_type *transition, bool ignore, bool abort);


// GOOD: Function signature requires more meaningful enum rather than bool
enum transition_abort_mode {
    TRANSITION_ABORT_INVALID, TRANSITION_ABORT_ALL,
    TRANSITION_ABORT_NEW, TRANSITION_ABORT_NONE
};


// Signature:
start_transition(transition_type *transition, enum transition_abort_mode);
```

* **Do not use unnecessary abbreviations** - if a class implements an "Advanced Output", call it AdvancedOutput, not AdvOutput. The same applies to variables holding an instance of the class. Code is read more often than it is written (and indeed the writing part is just the final outcome of a much longer reasoning process) and optimising for the latter incurs a debt on the former.
* **Name functions after what they do rather than how they do it** - a function’s signature is effectively its “interface” and should provide a hint about which functionality it provides to the caller. The actual implementation should not be relevant to the caller and exposing this information can actually have a net-negative effect as the caller might try to “out-think” the API.
* **Prefer pure functions and non-mutable variables** - side-effects are harder to keep track of and can lead to confusing results, particularly when following the prior rule. This also ties into a later rule about avoiding global scope, as global (and shared) variables tend to encourage writing code that surreptitiously changes global shared scope, which makes code harder to test and reason about.

> [!NOTE]
> This obviously does not apply to methods where the implementation detail is a technical need of the API user e.g., for factory methods where the user has data in a specific place that it needs to be loaded from, such as `MyData::loadFromUrl(const std::string &url)` or `MyData::loadFromFile(const std::filesystem::path &path)` where the “what” is essentially intertwined with the “how”.

* **Do not use specific integer types outside of binary protocols** - for the vast majority of cases using an “int” is sufficient and using a signed type also prevents unexpected overflow issues where a small negative number becomes a massive positive number. Use sized types (and unsigned types) when interacting with binary protocols, bitfields (e.g. to explicitly remove any special meaning from the most-significant bit). Prefer 64-bit integers over unsigned 32-bit integers for large numbers.
* **Do not micro-optimize and do not attempt to be overly "clever" with an implementation** - to quote Kernighan:

> Everyone knows that debugging is twice as hard as writing a program in the first place. So if you're as clever as you can be when you write it, how will you ever debug it?

* **Use profiling tools to identify actual opportunities for optimization** - the code that the compiler generates can potentially work in different ways than the source code might suggest and might not even be executed in the same order. Premature optimizations might also prevent the compiler from applying its own, leading to slower code overall. Also be mindful of profiling tools themselves potentially influencing the runtime behaviour of code.
* **Attempt to actively break code during testing** - do not rely on “happy paths”. Try to be creative and come up with ways to break the assumptions your code might have made. Do not rely on assumptions that data will never be “wrong” or that some other data will always be “there”. As the ISO C++ FAQ calls it:

> Write code that is guaranteed to work, not code that doesn’t seem to break.

* **Write code that will be reasonably easy to understand even 6, 12, or 24 months later** - while it is easy to reason about the state of a program while working on it, much of that “inside knowledge” might be gone even a few weeks later. Writing code that is obvious in what it does (and also why it does certain things and which pieces of data it requires) will make it easier to get back into the mental model behind the code. **Write code so that any newcomer to the project will have a reasonably easy time understanding what it does and why it does it the way it’s implemented.**
* **Do not rely on global state or singletons** - both are easily abused as shortcuts to avoid implementing a more expressive and safer design. There are valid use cases for global state (like a global “application” instance implemented as a singleton) but those are the exception and not the rule.
* **Treat programming as a mental exercise and not just as text manipulation** - existing code in particular will have been designed and built with a set of assumptions and theories in mind, which themselves informed the architectural choices made in its implementation. Some changes might seem “quick and easy” but <u>potentially violate those architectural assumptions</u> and thus could leave the code in a precarious state (and indeed might lead to undefined behaviour when yet other code relies on these assumptions not to be violated).

The last point cannot be overstated because it is a common source of friction in long-lived software projects. **Neither source code nor documentation can be a full representation of the “theory” behind the code** and indeed both can never be more than an incomplete “snapshot” of one possible implementation of this theory.

But without a decent understanding of the theory behind the code (or the architectural concerns that went into the current design) one cannot correctly ascertain which changes will be “in line” with the way current code works and might instead violate important principles of it.

And the more such “naive” code (mainly in the form of hacks and workarounds) is piled on, the more difficult the code becomes to work with, culminating in a code base where even shipping “simple” new features becomes a tough exercise (as the code starts to behave in unpredictable ways due to all the violations of the original “theory”).

> [!NOTE]
> This principle becomes more obvious with less abstract examples: An image-editing program will have a set of considerations and constraints that went into its design (code and user interface) and just adding the ability to decode video files will not make the program able to edit video, which is an entirely separate discipline with potentially opposing needs and considerations.
>
> And indeed any new feature added to the photo editor now has to potentially contend with the reality that it might be faced with a video file instead, adding even more bits and pieces to unrelated parts of the program to handle a scenario the foundation of the program was conceptually (and explicitly) not built for.

## Language Specific Guidance

OBS Studio currently contains code written in the following languages:

* C
* C++
* Objective-C/C++
* Swift
* CMake

Continuous integration code is mostly based on Powershell, Zsh or Bash scripts, and Python 3.

While C++ and ObjectiveC/C++ are supersets of the C language, the project prefers to treat them individually with their own rules, conventions, and language standards. Thus rules that apply to “C” do not necessarily apply to these languages and indeed some rules will be replaced for those languages.

### C

For “pure” C code the project follows the Linux Kernel Coding Style (https://github.com/torvalds/linux/blob/master/Documentation/process/coding-style.rst). Parts of the guidelines that relate to Emacs, kernel-level allocators, or macros only available in the Linux kernel source code do not apply.

> [!IMPORTANT]
> The current C language standard of the project is **C17 without GNU extensions**.

Some additional notes:

* Formatting is checked and can be applied by `clang-format`. The formatting generated by it supersedes any rules in the guidelines.
* The project uses a line length of 120 characters, and uses tabs for indentation with a tab width of 8 characters.

### C++

The project treats C++ as its own language and not just as “C with classes”. The associated code style guidelines are based on the Google C++ Code Style Guide (https://google.github.io/styleguide/cppguide.html) with changes to some aspects, as listed below.

> [!IMPORTANT]
> The current C++ language standard of the project is **C++17 without GNU extensions**. A move to C++20 is currently being considered.

Additions and changes (in the order the associated topics appear in the Google document):

* **Header Files**
    * OBS Studio uses the “cpp” suffix for C++ source code files rather than “cc” and “hpp” for C++-specific header files
    * Header guards are permitted, but not required - a simple `#pragma once` is sufficient
    * Use of forward declarations is permitted for class or struct types, but be mindful of the caveats mentioned in the Style Guide. **Avoid “Structuring code to enable forward declarations”.**
    * OBS Studio uses a different header include order, documented in a separate section below
* **Classes**
    * If any constructor or assignment operator is defined, all 5 types need to be either defined, defaulted, or deleted, including the destructor (**“Rule of Five”**)
    * Ensure that move constructor and move assignment operator are marked “noexcept” (and implemented accordingly) to avoid possible pessimization when using user-defined types with standard library containers like `std::vector`.
    * Use multiple-inheritance only for the interface/protocol pattern ("implements an interface as defined by" relationship), avoid the diamond pattern and virtual base classes.
* **Google-Specific Magic**
    * OBS Studio does not use `cpplint`
* **Other C++ Features**
    * Exceptions can (but don't have to) be used, particularly when no good "sentinel value" can be provided and also to avoid littering the code with correctness checks after every function call. It is preferred to keep the “scope” of exceptions within the same module (i.e., the same executable or library).
    * Be aware that in C++ <u>any</u> function that is not a destructor and is not explicitly marked as noexcept can potentially throw an exception as part of normal error handling rather than signaling an actual critical failure.
    * Instance methods that modify any data, even if it is not their own instance's data, should not be marked `const` to signal that they <u>logically</u> mutate some state.
    * Do not use comments to declare the types in structured bindings
    * Any guidelines of C++20 features will be evaluated once the project switches to that language standard.
    * The project does not allow the use of the Boost library.
    * However the project does allow the use of the “Disallowed standard library features” and some “Nonstandard Extensions”.
* **Naming**
    * Use the class name (following type name rules) for the file name of its interface header and implementation file.
    * Use `camelCase` for instance methods, functions, as well as variables. This also aligns with Qt’s code style.
    * Existing constants in code are permitted to be kept with `UPPERCASE` names, but new constants should follow the new convention of `kCamelCase`.
    * Namespaces need to follow type name rules, though the project only uses the “`OBS`” namespace for refactoring of application code for the time being.
    * Do not use underscore prefixes for any names, as these are reserved for the standard library implementation. Use a trailing underscore for private member variables.
* **Comments**
    * Only C++ comment style is permitted for C++ source code, mixing of different styles is not allowed.
    * Do not use function argument comments.
* **Formatting**
    * Formatting is checked and can be applied by `clang-format`. The formatting generated by it supersedes any rules in the guidelines.
    * The project uses a line length of 120 characters, and uses tabs for indentation with a tab width of 8 characters.
    * All new C++ code needs to use curly braces for all looping or branching statements. No exceptions.
    * Prefer brace initialization to protect against (unexpected) narrowing and prefer it over C-style assignment, except where a functional difference in behavior is needed (e.g. for `std::vector`)
    * Class visibility labels are not indented.

Some additional notes:

* Use the C++ standard template library and C++ algorithms as much as possible, but also be aware of known issues with some of them (e.g. bad runtime performance of `std::regex`).
    * **Prefer the standard library** over implementing custom loops.
    * Prefer **C++ collections and filesystem functions** over their C variants (e.g. `std::array` over C arrays)
    * Prefer **range-based and iterator-based loops** over counter-based loops.
    * **Prefer C++ types over C types**
    * Wrap C library code in C++ code to provide a clean C++ interface first.
    * **Use `class enum` instead of `enum`** for enumeration values and do not use enum as “integer” value aliases.
    * Do not use inline or static in the same way they are used in C

> [!IMPORTANT]
> The “`static`” keyword is one of the more confusing aspects of modern C++, particularly compared to C. The meaning of the keyword changes depending on whether it is used for a function, a global variable, a function-scope variable, a class method, or a class member.
>
> In general limit its use in C++ code to describe “storage duration” of variables or for class methods (e.g. factory methods). Use thread-safe functions to initialize a function-local static variable or class member (to prevent possible race conditions when the variable is initialized). Also be mindful of the “Static Initialization Order Fiasco”. Otherwise all the common established pitfalls and issues of global variables apply.
>
> `constexpr` definitions do not need to use the static keyword. `constexpr` implies const and const implies static storage duration by default.
>
> Be careful when mixing `static` and `inline`, as the inline keyword (short for “defined in-line”) allows the same definition to exist in multiple translation units (and thus is allowed to violate the one-definition rule, or “ODR”), enabling the linker to deduplicate all instances of the same function, which is the exact opposite of what “`static`” requires (local visibility and thus a distinct copy of the function in each translation unit).

#### The Exception: Qt Guidelines

Many of Qt’s core concepts were invented years before C++ added support for similar ideas, which means that much code interacting with Qt library functions and QObject-based instances requires violating some of the core language principles outlined above:

* Qt's ownership model predates modern smart pointers or references and thus requires passing around raw pointers:
    * Any class derived from QWidget needs to be instantiated using bare new.
    * Parent QObjects take ownership of children and will take handle destructing them appropriately
    * Thus: **Do not call `delete` on a widget owned by a parent widget. Do not create widgets without attaching them to a parent widget.**
* Qt's default string class QString was an early adopter of Unicode (like Windows or Cocoa on macOS) and uses a 2-byte encoding internally (which was adapted to become UTF-16)
    * A QString needs to be converted between UTF-16 and UTF-8 when passing character data to any C-based API or when interacting with `std::string` instances.
    * **There Ain’t No Such Thing As Plain Text** (https://tonsky.me/blog/unicode/)
* The lifetime of a heap object passed to Qt must be guaranteed to match or exceed the lifetime of the using/owning Qt object.
* Lambda expressions can and should be used for callback-style programming as much as possible.
    * **Be aware of lifetime issues however**: Any reference captured by a lambda needs to be "alive" for as long as Qt might potentially invoke the lambda expression. Use copy-based capture for scalar values.
    * Note that a pointer is "just" a scalar value and is thus captured by copy, but there is no strong relationship between the copied pointer and the memory it is pointing to. The lifetime of the heap object needs to be ensured. Be mindful of the lifetime of whomever will be invoking the lambda.
    * A pointer captured by reference can potentially be set to nullptr and checked within the lambda expression's body, but the lifetime of the pointer reference itself now needs to be ensured.

### C/C++ Header Include Order

Prefer to include the headers that the implementation itself needs (e.g. because a type or definition is used directly in the implementation) and do not rely on another header possibly having included the same file already. **This follows the “include what you use” rule**.

When including headers, use the following order:

```C++
// Interface definition or “counterpart” of current file
#include “interface.h”


// File in the same directory as the current file _if_ header belongs to the
// same “implementation”.
#include “file_in_working_directory.h”


// First party dependency from the same larger project that is “linked” with
// the implementation
#include <first_party_dependency/type_or_interface.h>


// Third party dependency not part of the same project and “linked” with the
// implementation.
#include <third_party_dependency/type_or_interface.h>


// C++ standard library includes
#include <string>


// C standard library includes
#include <sys/socket.h>
```

> [!NOTE]
> This specific order of includes helps in identifying potentially “broken” headers without “masking” the issue by including potential dependencies first.
>
> That issue can be avoided by following a simple procedure when creating a new interface and implementation pair: The initial implementation should always include the “counterpart” interface header first and should compile just fine even with the “empty” implementation.
>
> The same applies to any first party and third party library headers: Including one by itself should not result in compilation issues or should not require any standard library headers being included first (if that’s the case, the corresponding library header is badly designed).
>
> Putting the standard library includes last (and only including them if the implementation needs them) helps expose such malformed header files. While this scheme relies on convention (rather than enforcement by the language) it leads to self-contained headers and a cleaner set of includes.

Additional rules for this guideline:

* Includes within each block need to be sorted alphabetically with case-sensitivity enabled.
* Do not create platform specific “include blocks”. If a single source file has to include a hodgepodge of platform-specific headers it usually suggests that the source file tries to do “too much” and the “uglyness” that the rule might lead to is the point. To quote the Google C++ Style Guide:<br ><br />**“Instead of using a macro to conditionally compile code ... well, don't do that at all”.**<br /><br />
* Refactor the source file to be platform-specific and use CMake to add the appropriate file to the target for a specific platform.
* Do not use double quotes to include any file not in the same working directory as the current source or header file. Compilers have been lenient and automatically attempt to use header include paths for all header files, so files might indeed compile nonetheless, but the additional context for the reader (“that header is provided by some other module/target in the project”) is lost.

Some header files will require breaking these rules. Well-known examples include:

* `windows.h` might be required very early in an implementation, particularly when relying on the magic `WIN32_LEAN_AND_MEAN` define, as nested includes of the same header might break compilation in severe ways otherwise..
* The BSD header `libprocstat.h` requires additional headers to be included in a specific order before its own inclusion (as documented in its man page).
* If an impact of include order can be proven through testing, additional headers are allowed to break these rules. This is unavoidable given the legacy and preprocessor architecture of C and C++.

### Objective-C/C++

For Objective-C/C++ code the Google Objective-C Style Guide (https://google.github.io/styleguide/objcguide.html) should be followed, which itself is based on Apple’s [Cocoa Coding Guidelines](https://developer.apple.com/library/content/documentation/Cocoa/Conceptual/CodingGuidelines/CodingGuidelines.html) and [Programming with ObjC Conventions](https://developer.apple.com/library/archive/documentation/Cocoa/Conceptual/ProgrammingWithObjectiveC/Conventions/Conventions.html).

> [!IMPORTANT]
> The current Objective-C/C++ language standard of the project is **Objective-C 2.0**.

The additions and changes to the Google Style Guide are as follows (in the order the associated topics appear in the Google document):

* **Naming**
    * Use `camelCase` for functions.
    * Use of the `g` prefix for global variables in file scope is permitted.
* **Types**
    * Always use the native 64-bit types (i.e., double for CGFloat).
* **Comments**
    * Use Apple’s DocC format for documentation (https://www.swift.org/documentation/docc/).
    * Use C++ comment style for inline comments.
* **Cocoa and Objective-C Features**
    * Use `#import` for all includes, follow the include order as specified for C/C++.
* **Spacing and Formatting**
    * Formatting is checked and can be applied by `clang-format`. The formatting generated by it supersedes any rules in the guidelines.
    * Use 4 spaces for indentation, use spaces for alignment.
    * The maximum line length is 120 characters.
    * All code needs to use curly braces for all looping or branching statements. No exceptions.

### Swift

For Swift code the Google Swift Style Guide (https://google.github.io/swift/) should be followed as well as Apple’s Swift API Guidelines (https://www.swift.org/documentation/api-design-guidelines/).

> [!IMPORTANT]
> The current Swift language standard of the project is **Swift 6**.

The additions and changes to the Google Style Guide are as follows (in the order the associated topics appear in the Google document):

* **General Formatting**
    * Formatting is checked and can be applied by `swift-format`. The formatting generated by it supersedes any rules in the guidelines.
    * Use 4 spaces for indentation, use spaces for alignment.
    * The maximum line length is 120 characters.

Some additional notes:

* **Use the Unmanaged type and associated protocols to create retained or unretained opaque pointers to share with C APIs.**
    * Use `passRetained` to pass an opaque pointer with an incremented reference count to a C API.
    * Use `takeRetained` to take ownership of that reference count in Swift code and allow normal lifetime management to take over.
    * Use `passUnretained` and takeUnretained to pass/receive pointers without influencing their reference count.
    * **Object lifetime has to be manually ensured, and is thus unmanaged**
* Use extension to implement protocols in code blocks separate from the core type implementation.

```Swift
class MyType {
    // Basic implementation
    fileprivate let memberVariable: String


    init(argumentOne: String) {
        self.memberVariable = argumentOne
    }
}


extension MyType : SomeProtocol {
    func someProtocolMethod(argument: String) -> String {
        return “\(memberVariable) \(argument)”
    }
}
```

* **Prefer functional patterns as much as possible over C-style loops.**
    * When a loop is still preferable, use the native Range type and enumerated loops if a counter value is required:

```Swift
for i in 0..<someValue {
    doTheThing()
}


for (i, theThing) in theCollection.enumerated() {
    doTheThing(with: theThing);
    print("I am on iteration \(i) here...")
}
```

### Objective-C/C++ and Swift

* **Be mindful of reference counts** when sharing Objective-C/C++ or Swift object instances or their data with external code:
    * Explicitly increment the reference count on an object to ensure its not deallocated when the current function scope is left and to ensure that the pointer shared with C code is kept alive
    * Balance that reference count on an object when the external code calls for its "destruction" by decrementing the reference count - **do not manually deallocate the object**, instead let its reference count go to `0`
* Wrap code that potentially creates a lot of new heap objects in autoreleasepools within a loop body to allow the reference counts to be evaluated early within each iteration
    * This can potentially help keeping the memory footprint of each loop iteration at the same level
* Prefer language specific collection and string types over C array or character pointers.
    * For Objective-C/C++, prefer `NSString`, `NSDictionary`, `NSArray`, `NSSet`, and others
    * For Swift, prefer `String`, `Dictionary`, `Array`, `Set`, and others
    * Swift and Cocoa types are bridged and interoperable
* **Prefer Objective-C/C++ blocks and Swift closures over lambda expressions**

### CMake

CMake underwent a big philosophical change with version 3 of the build system generator, which established the notion of “targets” with associated “target properties” that describe a target’s needs, including compiler arguments, linker flags, preprocessor definitions, and more.

The project’s build system was rewritten a few years ago to make full use of these “modern” CMake patterns and approaches, and thus requires all new CMake code to follow the same principles:

* Refer to “Modern CMake” (https://cliutils.gitlab.io/modern-cmake/README.html) for general guidelines about how to write modern CMake code.
    * **Always prefer targets** and the use of target properties over magic global variables
    * **Prefer generator expressions** over explicit branches in CMake code, if it makes the overall code clearer
* **Use 2 spaces for indentation** and spaces for orientation
* Use `UPPER_SNAKE_CASE` for cache variables, global variables, and file-local variables
    * Use `snake_case` for function names and function-local variables
    * Use an `_UNDERSCORE_PREFIX` for "private" variables that need to exist in the global or file-local scope

> [!NOTE]
> Macros differ from functions in that they do not have a function-local scope. A macro body is effectively put into the scope it is called from and thus shares the variable scope.

* Pass variables to functions by their name, use `${}` dollar expansion to pass their value
* **Always use double quotes for strings**
* Prefer using functionality either available via CMake itself or available for specific generators over custom code that runs at configure time
    * Particularly when multi-config generators for Visual Studio or Xcode are used, code running at configure time does not have access to the "actual" build configuration that will be used by the IDE and thus is incapable of achieving its potential goal anyway.

It is important to remember that CMake does not represent “the build system”, but rather is a “build system generator” that translates the abstract dependency tree of “targets” into an actual build system or IDE project.

### JSON and YAML

Both file formats are predominantly used for build system configuration or continuous integration, but are still subject to a limited set of rules:

* **Use schema files as much as possible** to ensure that a given JSON or YAML file uses correct structure and - as far as possible - valid values.
    * Some editors can pull up common schema files when a loaded JSON or YAML file has a specific file name or sits in a directory of a specific name.
* Use `camelCase` for variable names
    * **Exception:** Job names for GitHub action workflows commonly use `dash-case` for the job names.
    * **Exception:** If the key of a variable is used for an underlying system e.g., to define environment variables in a shell environment that commonly uses `UPPER_SNAKE_CASE` names.
* Use **double quotes** for **key names** and **string values** in JSON files.
* Use **single quotes** for **complex strings in YAML files** that might otherwise not be correctly interpreted as strings, quoting is otherwise not necessary.

Be aware of the different multi-line string behaviors available in YAML:

* `'Single quoted'` strings ignore leading white space (aka indentation), escaped line-breaks like `\n` and require two actual line breaks to add an actual new line.
* `"Double quoted"` strings support escaped line-breaks, but otherwise behave like singled-quoted strings.
* `Unquoted` strings behave like single-quoted strings, but YAML might give specific words or letters special meaning and can thus lead to unexpected behavior.

It is safer to use "block scalars" that make the use of multi-line strings (and how they should be parsed) more explicit:

* The `|` block-style indicator will retain newlines and indentation as-is.
* The `>` block-style indicator will _replace newlines with spaces_.
* By default both indicators will retain a single newline at the end of the string.
* If the `-` indicator is added, _no newline is added_.
* If the `+` indicator is added, _all existing newlines_ at the end of the string are retained.
* Additionally an "indentation" number can be added to declare the number of spaces used for indentation by the string.

#### GitHub Actions Workflows And Actions

Use [Zizmor](https://zizmor.sh) to lint any changes to GitHub Actions workflow or repository action files.

> [!IMPORTANT]
> Use `zizmor --no-online-audits --persona=auditor`.

Repository actions use strings for input and output and while GitHub will automatically convert native YAML types into JSON string representations before providing them as inputs, the same does not happen for outputs.

To use such outputs as boolean values, use the `fromJSON()` function to convert the (JSON) string into its actual type. This can also be used to convert a JSON array string back into an actual array (which can then be used by functions like `contains()`).

Use block scalars (see above) to break up long strings in workflow syntax into more readable multi-line variants. Combined with the `format()` function, this allows for complex string generation or conditionals while keeping the code readable:

```YAML
      - name: Some Workflow Action
        if: >-
          fromJSON(env.SOME_VARIABLE)
          && steps.some-step.outputs.some-output == 'hello'
        uses: actions/some-action
        with:
          path: >-
            ${{ format(
                '{0}/my-desired-file-{1}-{2}-output.{3}',
                runner.temp, github.sha, steps.some-step.output.some-output, case(
                    runner.os == 'Windows', '.zip', '.tar.xz'
                )
            ) }}
```

This example ensures that the string passed to the action contains no line breaks (which can lead to unexpected script execution issues) and avoids having the expression being cramped into a single line.

### Shell Scripts

Shell scripts are used in two areas: GitHub Actions workflows and actions, and additional helper scripts in the `build-aux` directory.

Scripts for local use should use the scripting language most common on their target platform:

* Zsh scripts for macOS
* Bash scripts for Linux, BSDs, and others
* Powershell Core scripts for Windows.

While Zsh and Bash share a lot of common ground, they differ in important ways that impact the way scripts can be designed for them, while the Powershell script language is a much more powerful object-oriented language.

#### Scripts Used On CI

Scripts can be used in two forms in GitHub Actions: Either as inline snippets in `steps` or as full scripts invoked directly. In the former variant, GitHub Actions will take the text body defined inline and copy it into a script file on the runner's drive, before invoking it just like in the second form.

By default Bash is preferred for scripts used on CI, with Zsh scripts being allowed for macOS-exclusive implementations. Powershell scripts should be avoided.

> [!NOTE]
> Powershell's debug output is severely flawed compared to Bash's and Zsh's:
> * The output is more verbose and does not just print out the script line that is being executed.
> * This makes the output harder to parse and read in GitHub Actions logs when debugging workflows and actions.
> * Repository and environment secrets are automatically masked by GitHub Actions when printed in their entirety. But Powershell truncates its log output automatically.
> * This can lead to partial leakage of secrets as the values cannot be masked by GitHub Actions's output parsing.
> * Neither Bash nor Zsh are affected by this.

The following rules apply to all scripting languages when used in GitHub Actions workflows and composite actions:

* Ensure the validity of environment variables
    * It is good practice to provide inputs to shell scripts as environment variables, but their definition as well as their values cannot be assumed.
    * It is allowed to assume that default environment variables like `GITHUB_OUTPUT`, which are part of the GitHub Actions CI environment are set.
    * For all other environment variables, use appropriate measures to ensure that they are actually bound or use fallback values:
        * Use `set -o nounset` (or `setopt NO_UNSET` in Zsh) to automatically abort script execution if an _unset_ variable is encountered.
    * Abort script execution outside of inline snippets if the "magic" environment variable `CI` is not set.
    * Use **Shell Parameter Expansion** in [Bash](https://www.gnu.org/savannah-checkouts/gnu/bash/manual/bash.html#Shell-Parameter-Expansion-1) and [Zsh](https://zsh.sourceforge.io/Doc/Release/Expansion.html#Parameter-Expansion) to handle potentially unset or empty variables.
        * Use `${VARIABLE:?}` to have the script fail immediately if an environment variable is not set or has an empty string value explicitly (rather than have the script fail at first access due to `NO_UNSET`).
        * Use `${VARIABLE:=default}` to initialize an empty or unset variable with a default value.
        * Use `${VARIABLE:-default}` to temporarily use a fallback value if the variable is unset or empty.
* Always wrap the main functionality of a script in a separate function, preferable with the name of the script itself, and call this function from the global scope.
* Use GitHub Actions output modifiers like `::warning::` and `::error::` to make them stand out.
    * For actual errors, return `1` as the error code to "fail early".
* Do not assume the current working directory of a script holds a checkout of the project.
    * The working directory of a script can be changed in multiple ways and can also be set explicitly by a workflow or composite action before the script is invoked.
    * Design composite actions in such a way that they can either be run from any working directory (whether that yields any benefit is up the caller) or fails early if the working directory does not fulfill specific requirements (e.g. a necessary file or directory is present).
    * A common pattern is to provide callers a way to set the working directory for a composite action (e.g. if the checkout has been placed in an alternative location) but use `github.workspace` as the default value, which aligns with the checkout action's default behavior. Scripts associated with the action should still run checks to confirm the working directory indeed contains the required file(s).
    * Design scripts in a way that they only pass absolute file paths to commands and functions that support them, removing the working directory as a source of failure.
    * If the working directory needs to be adjusted (e.g. to create `tar` archives with relative paths), use `pushd` and `popd` to set and restore the working directories _with absolute paths_. This ensures that the script switches from whichever current working directory and also returns back to it before continuing the script.
* Scripts used for GitHub Actions do not need elaborate argument or error handling, as they are not meant for "human consumption".
    * Error and warning messages should be designed to be easily digestible at GitHub Action's workflow summary page.
    * Script and function arguments are not required as script inputs are realized using environment variables, which are checked per the rules above.
* Scripts should enable tracing when the `RUNNER_DEBUG` environment variable is set:
    * Bash: `set -x`
    * Zsh: `setopt XTRACE`

#### Bash

Bash scripts should try to target **Bash 3.0** but are allowed to use Bash 5.0 features if contained in an action that can automatically install or upgrade the necessary Bash version before running any actual scripts.

In general each shell script should follow the best practices outlined by [ShellCheck](https://www.shellcheck.net) and scripts (as well as script snippets used in GitHub repository actions) should be checked with it.

> [!IMPORTANT]
> Use `shellcheck --severity=style --shell=bash --enable=all` when linting Bash scripts.

* Ensure scripts themselves fail early on failures by external commands or nested function calls
    * Use the common practice of enabling `errexit`, `pipefail`, and `nounset` for all scripts.
    * Use conditional blocks or list operators like `&&` and `||` to gracefully handle non-zero return codes.
* Use double quotes for all variables by default, be aware of necessary exceptions.
    * Bash uses automatic word expansion and globbing on all variables by default, which can introduce side-effects. Thus it is best practice to always wrap variables in double quotes like so: `"${VARIABLE}"`.
* The exception to this rule is composition of glob expressions:
    * When composing a glob expression from one or more variables, avoid quotes for the parts that are intended to be subject to glob expansion, e.g. `"${path_prefix}"/some_path/*.txt`. Otherwise Bash will simply use the literal string `*.txt` and not expand it.
    * Use `compgen` when handling user-provided glob expressions.
* When the contents of a variable should be split into an array, use `read -a array_name <<< "${variable}"` instead of direct declaration (`declare -a array_name=(${variable})`) (ShellCheck will commonly suggest this automatically).
* Prefer built-in shell functionality over use of external commands as much as possible except if a shell built-in has known deficiencies or its capabilities are too limited and resulting code becomes harder to reason about.
    * Use glob expressions for simple matching, use POSIX regular expressions with `BASH_REMATCH` otherwise, use `sed` with extended regular expressions as a last resort.
* Be aware of the pitfalls of arithmetic expressions, as even the judicious use of double quotes will not prevent a command substitution from executing.
    * The test `$(( "${x}" ))` with `x` set to `a[0$(uname>&2)]` will execute `uname`. This is an existing issue with arithmetic evaluations in all modern shells.
    * To make matters worse, several builtin commands will automatically evaluate variables as arithmetic expressions depending on their invocation. E.g. `[[ "${x}" -lt 2 ]]` is converted into an arithmetic comparison and thus `x` is evaluated accordingly (and runs `uname`). The same applies when assigning `x` to a variable declared as numeric (e.g. `typeset -i a; a="${x}`).
    * Always sanitize any user-provided value that is directly or indirectly used in an arithmetic expression.
        * This includes the use of `(( ))`, `$(( ))`, `[[ ]]` when a numerical comparison is used (e.g. `-lt`, `-gt`, etc), setting numerical variables (`let`, `typeset -i`), or as indices for arrays. **Double quoting the variable will not prevent this**.

#### Zsh

Zsh scripts require **Zsh 5.8 or newer**, which should align with the build requirements on macOS. Due to its differences to Bash and its cousins, ShellCheck cannot be used to lint Zsh scripts. In general a large set of ShellCheck best practices do apply to Zsh scripts as well, with some differences.

* Ensure that scripts themselves fail early on failures by external commands or nested function calls.
    * Use `setopt` to set `ERR_EXIT`, `ERR_RETURN`, `PIPE_FAIL`, `NO_UNSET`, and also `WARN_CREATE_GLOBAL`, and `WARN_NESTED_VAR` for all scripts.
    * Use conditional blocks or list operators like `&&` and `||` to gracefully handle non-zero return codes.
* Ensure that scripts run in `zsh` mode by calling `builtin emulate -L zsh` as early as possible to prevent emulation of other shells' behavior.
* This enables Zsh's default behavior which will **not automatically expand variables**. Indeed Zsh only does so if specific modifiers are used. By default any variable stays a string, even if it could be split into words or its contents could be interpreted as a glob expression.
    * To have Zsh interpret a string as a glob expression, it needs to be written as `${~variable}`.
    * This effectively enables the `GLOB_SUBST` shell option for the expansion of this variable only. Do not enable `GLOB_SUBST` in a script if you want to make use of this behavior.
    * Likewise to have Zsh split a string per default shell rules, it needs to be written as `${=variable}`.
    * This can be used to split a variable containing words into an array via `typeset -a array=(${=variable})`, otherwise `typeset -a array=(${(s: :)variable})` or `read -A array <<< "${variable}"` can be used. Either way, the splitting needs be requested explicitly.
* While this reduces the potential impact of unquoted expansions, it does not eliminate them entirely:
    * Command substitutions (e.g. `$(some-command some_argument)`) are still broken into words using the `IFS` parameter in Zsh, so those should always be quoted (e.g. `output="$(some-command some-argument)"`).
    * Zsh (like other shells) elides empty variables if they are not enclosed in double quotes. This can lead to problems if variables are used with commands that have a strict requirement for positional arguments like `printf`.
    * Combined with the arithmetic evaluation issue shared with other shells, this can lead to unforeseen command execution. Consider `printf '[%d] %s'` which requires an even number of arguments. If provided with `1 ${empty_variable} 2 ${malicious_variable}`, the empty variable is elided and the `malicious_variable` is now used as input for `%d`, which will trigger arithmetic evaluation.
        * If `malicious_variable` is set to the string `psvar[0$(uname>&2)]`, `uname` will be executed. Note that double quoting will **not** prevent the execution, but if `empty_variable` were enclosed in double quotes, it would not have been elided and `malicious_variable` would not be subject to arithmetic evaluation.
* Thus the following rules apply for Zsh scripts:
    * Always use double quotes for command substitutions (`result="$(some_command)"`).
    * Always use double quotes when composing strings with parameter expansion (`string="some_${other_string}"`).
    * Always use double quotes to ensure that even an empty argument is passed to a command or function that _requires_ a positional argument (`some_command "${possibly_empty}"`).
    * Always sanitize any user-provided value that is directly or indirectly used in an arithmetic expression
        * On top of the examples mentioned for Bash, Zsh will implicitly use arithmetic evaluation for `printf`, `integer`, and `exit`.

#### Powershell

Powershell scripts require **Powershell Core 7.3 or newer**. In general Powershell scripts should be checked using `Invoke-ScriptAnalyzer` available in the `PSScriptAnalyzer` module. Some basic rules (which accommodate the requirements by the script analyzer) are:

* Always use a `[CmdletBinding]`.
* Prefer named parameter bindings over the use of `$Args`.
* Each function needs to have at least a `process` block. Use `begin` for setup code, use `end` for cleanup.
* Use `Verb-PascalCase` for function names, and use `PascalCase` for variables.
    * Environment variables commonly use `CAPITALCASE` but are wrapped in the `Env` object on Powershell.
* Splat arrays into arguments when passed to functions or other commands via `@ArrayVariable`.
    * Pack command arguments into an array if the command line would exceed a column limit of 120 characters and splat it in the invocation.
* Powershell has much less implicit expansion features than POSIX shells but provides a more extensive and modern set of tools for splitting, matching, and evaluations. Those features need to be invoked manually however.
    * While POSIX shells operate in a string-based manner, Powershell is object-based. The majority of first-party commands actually return an object that usually has a `ToString` method which is then implicitly called by the interpreter when passed to any other command that potentially takes a `String` as input.
    * This allows Powershell scripts to pass complex objects between commands using the `|` operator, and allows sub-commands to access all the instance properties and methods present on the object rather then just operating on a textual representation.
* Use functional patterns via piping as they are more canonical in Powershell:
    * As an example, `Get-ChildItem` can get a list of file entries. While this command can be supplied with a limited set of inclusion, exclusion, and filter arguments, they each have very specific behaviors.
    * Thus it is easier to pipe its output to `Where-Object` which is called for each "child item" and either returns `$true` or `$false` depending on whether the item fulfills a requirement. Similarly `ForEach-Object` is called for each item and can transform the input into a new output, which then replaces the original item.
    * `Get-ChildItem | Where-Object { $_.Name -match 'My Desired Name Prefix .+' } | ForEach-Object { $_.Name.Uppercase() }` is thus similar to patterns like `list.filter( _ =~ "My Desired Name Prefix .+" }.map( to_upper(_)` in other languages.
* When comparing an object against `$null`, always use the null constant first: `if ($null -eq $MyVariable)`.
