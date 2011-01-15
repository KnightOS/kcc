import sys, re
import string

"""Simple script that scans all of the test suite results text fed in
through stdin and summarises the total number of failures, test
points, and test cases."""

# Read in everything
lines = sys.stdin.readlines()

# Init the running totals
failures = 0
cases = 0
tests = 0
bytes = 0
ticks = 0
invalid = 0

# hack for valdiag
name = ""

for line in lines:
    # --- Running: gen/ucz80/longor/longor
    m = re.match(r'^--- Running: (.*)$', line)
    if (m):
        name = m.group(1)

    # '--- Summary: f/t/c: ...', where f = # failures, t = # test points,
    # c = # test cases.
    if (re.search(r'^--- Summary:', line)):
        (summary, data, rest) = re.split(r':', line)
        (nfailures, ntests, ncases) = re.split(r'/', data)
        failures = failures + string.atof(nfailures)
        tests = tests + string.atof(ntests)
        cases = cases + string.atof(ncases)
        if (string.atof(nfailures)):
            print "Failure: %s" % name

    # '--- Simulator: b/t: ...', where b = # bytes, t = # ticks
    if (re.search(r'^--- Simulator:', line)):
        (simulator, data, rest) = re.split(r':', line)
        (nbytes, nticks) = re.split(r'/', data)
        bytes = bytes + string.atof(nbytes)
        ticks = ticks + string.atof(nticks)

    # Stop at 0x000228: (106) Invalid instruction 0x00fd
    if (re.search(r'Invalid instruction', line)):
        invalid += 1;
        print "Invalid instruction: %s" % name

if (invalid > 0):
    print "%d invalid instructions," % invalid,
print "%.0f failures, %.0f tests, %.0f test cases, %.0f bytes, %.0f ticks" % (failures, tests, cases, bytes, ticks)
