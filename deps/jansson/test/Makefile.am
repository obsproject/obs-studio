SUBDIRS = bin suites
EXTRA_DIST = scripts run-suites

TESTS = run-suites
TESTS_ENVIRONMENT = \
	top_srcdir=$(top_srcdir) \
	top_builddir=$(top_builddir)

clean-local:
	rm -rf logs
