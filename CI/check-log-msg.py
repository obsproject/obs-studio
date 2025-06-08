#! /bin/env python3

import git
import re

title_limit = 72
title_text_limit = 50
title_text_re = re.compile(r"([A-Z][a-z]*(-[a-z]+)*|Don't) .*[^.]$")
description_line_limit = 72
description_line_ignore_re = re.compile(r'(^Co-Authored-By:|^ *(\[[1-9][0-9]*\] |)https?://[^ ]*$)')

LOG_ERROR = 100
LOG_WARNING = 200
LOG_INFO = 300
loglevel_threashold = LOG_WARNING


def find_directory_name(name, tree, max_depth):
    for t in tree.trees:
        if name == t.name:
            return True
    if max_depth > 1:
        for t in tree.trees:
            if find_directory_name(name, t, max_depth - 1):
                return True
    return False


def check_path(path, tree):
    paths = path.split('/', 1)
    name = paths[0]
    if len(paths) == 1:
        return find_directory_name(name, tree, 1)
    else:
        for t in tree.trees:
            if name == t.name:
                return check_path(paths[1], t)
    return False


def find_toplevel_file(name, blobs):
    for b in blobs:
        n = b.name
        if n[0] == '.':
            n = n[1:]
        n = n.split('.', 1)[0]
        if name == n:
            return True


def find_submodule_name(name, submodules):
    for s in submodules:
        sname = s.name.split('/')[-1]
        if name == sname:
            return True


class CommitFromJson:
    '''Helper class to provide the same interface as git.Commit.'''
    def __init__(self, obj, repo):
        self._obj = obj
        self.message = obj['commit']['message']
        self.hexsha = obj['sha']
        self.repo = repo
        # Below is not exactly same as git.Commit but provided because better than nothing.
        self.tree = repo.tree()


class Checker:
    def __init__(self, repo):
        self.has_error = False
        self.current_commit = None
        self.repo = repo

    def blog(self, level, txt):
        if level > loglevel_threashold:
            return
        if level == LOG_ERROR:
            self.has_error = True
            s_level = "Error: "
        elif level == LOG_WARNING:
            s_level = "Warning: "
        elif level == LOG_INFO:
            s_level = "Info: "
        print('{level}commit {hexsha}: {txt}'.format(level=s_level, hexsha=self.current_commit.hexsha[:9], txt=txt))

    def check_module_names(self, names):
        for name in names:
            if name.find('/') >= 0:
                if check_path(name, self.current_commit.tree):
                    return True
            else:
                if find_directory_name(name, self.current_commit.tree, 3):
                    return True
                if find_toplevel_file(name, self.current_commit.tree.blobs):
                    return True
                # TODO: Cannot handle removed submodules. Implement to parse .gitmodules file.
                if find_submodule_name(name, self.current_commit.repo.submodules):
                    return True
        self.blog(LOG_ERROR, "unknown module name '%s'" % name)

    def check_message_title(self, title):
        title_split = title.split(': ', 1)
        if len(title_split) == 2:
            self.check_module_names(re.split(r', ?', title_split[0]))
            title_text = title_split[1]
        else:
            title_text = title_split[0]

        if len(title) > title_limit:
            self.blog(LOG_ERROR, 'Too long title, %d characters, limit %d:\n  %s' % (len(title), title_limit, title))
        elif len(title_text) > title_text_limit:
            self.blog(LOG_WARNING, 'Too long title excluding module prefix, %d characters, recommended %d:\n  %s' % (
                len(title_text), title_text_limit, title_text))

        if not title_text_re.match(title_text):
            self.blog(LOG_ERROR, 'Invalid title text:\n  %s' % title_text)
            self.has_error = True

    def check_message_body(self, body):
        for line in body.split('\n'):
            if description_line_ignore_re.match(line):
                continue
            if len(line) > description_line_limit and line.find(' ') > 0:
                self.blog(LOG_ERROR, 'Too long description in a line, %d characters, limit %d:\n  %s' % (
                    len(line), description_line_limit, line))
        pass

    def check_message(self, c):
        self.current_commit = c
        self.blog(LOG_INFO, "Checking commit '%s'" % c.message.split('\n', 1)[0])

        msg = c.message.split('\n', 2)
        if len(msg) == 0:
            self.blog(LOG_ERROR, 'Commit message is empty.')
            return True

        if re.match(r'Revert ', msg[0]):
            return False
        if re.match(r'Merge [0-9a-f]{40} into [0-9a-f]{40}$', msg[0]):
            return False
        if re.match(r'Merge pull request', msg[0]):
            return False

        if len(msg) > 0:
            self.check_message_title(msg[0])
        if len(msg) > 1:
            if len(msg[1]):
                self.blog(LOG_ERROR, '2nd line is not empty.')
        if len(msg) > 2:
            self.check_message_body(msg[2])

    def check_commits(self, commits):
        for c in self.repo.iter_commits(commits):
            self.check_message(c)

    def check_by_json(self, commits):
        for obj in commits:
            c = CommitFromJson(obj, self.repo)
            self.check_message(c)


def open_or_stdin(name):
    if name == '-':
        import sys
        return sys.stdin
    else:
        return open(name)


help_epilog = """
examples:
  Check commit messages taken from GitHub REST API:
    gh api repos/obsproject/obs-studio/pulls/5248/commits > commits.json
    %(prog)s -j commits.json
  Check commit messages from git log:
    %(prog)s -c origin/master..
"""


def main():
    import sys
    import argparse
    parser = argparse.ArgumentParser(prog=sys.argv[0],
                                     description='Log message compliance checker',
                                     formatter_class=argparse.RawDescriptionHelpFormatter,
                                     epilog=help_epilog)
    parser.add_argument('--commits', '-c', default=None, help='Revision range')
    parser.add_argument('--json', '-j', default=None, help='JSON file to take the log messages')
    parser.add_argument('--verbose', '-v', action='count', default=0, help='Increase verbosity')
    parser.add_argument('--quiet', '-q', action='count', default=0, help='Decrease verbosity')
    args = parser.parse_args()

    global loglevel_threashold
    loglevel_threashold += args.verbose * 100 - args.quiet * 100

    checker = Checker(git.Repo('.'))

    if args.commits:
        checker.check_commits(args.commits)

    if args.json:
        with open_or_stdin(args.json) as f:
            import json
            obj = json.load(f)
            checker.check_by_json(obj)

    if checker.has_error:
        sys.exit(1)


if __name__ == '__main__':
    ret = main()
