# Tests #

## Adding a test ##
Tests are separated into suites. This is to prevent having to re-run all tests when you only want to test one particular feature. When adding a test, try and choose the most appropriate suite (which is organized by folders) the test sits in and add the test to the corresponding tests.c file. You will need to understand a bit of the cmocka API which can be found [here](https://cmocka.org/).

If adding a new suite, be sure to add an appropriate line to `suites/CMakeLists.txt` and to add a `CMakeLists.txt` script in the suite folder itself. An addition to `main.c` will also need to be made in order to run the exposed tests. See `suites/startup/tests.c` for an example.

```
─tests
  ├CMakeLists.txt
  └suites
    ├CMakeLists.txt
    ├startup
    │ ├CMakeLists.txt
    │ ├tests.c
    │ └tests.h
    └<new suite>
      ├CMakeLists.txt
      ├tests.c
      └tests.h
```

## How to compile and use ##
By default, the tests are not built. In order to build, you need to pass CMake `BUILD_TESTS:BOOL=true`. When the tests are compiled, they're automatically installed into the `rundir` directory. Right now, there isn't any install logic so trying to pack it up with CPack or `cmake --build . --target install` will not include it. To run, simply run `libobs_tests` in the rundir directory.

## Limitations ##
The tests are currently limited to the interface that libobs exposes making it a bit limited on what it can actually test. That's pretty normal but a bit inconvenient if we wish to unit-test some of the more complicated internal functionality.

Plugins can also only be tested by either having a dummy registration functionality that's yet to be made or by whatever interface that libobs exposes in general. If a plugin wants internal unit-test'ing, it can't use this unless they expose the functionality they wish to test.

This only pertains to API functionality, it doesn't contain any testing abilities that integrate into Qt. Some [QTest](http://doc.qt.io/qt-5/qtest-overview.html) use may help but I've not played with it myself.

## Issues ##
I've tested the `Visual Studio` generators and the `Ninja` generators on Windows. Anything else is currently untested.

I chose to put all tests of each suite into one file. That decision helps keep building a bit simple but might cause a cluttered test file. That decision may need to change if it turns out to be a problem.

This is the beginning of a very long road for testing that probably should have been started when the project started. There's a long way to go. 