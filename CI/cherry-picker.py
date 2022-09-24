import os
import sys

from subprocess import check_output, STDOUT, CalledProcessError

import requests


CHERRY_PICK_TAG_PREFIX = 'cherry-pick-to-'
PR_MESSAGE = '''Cherry Picker results:
{}

Errors:
{}

Created by workflow run https://github.com/{}/actions/runs/{}
'''


def get_pr_targets(s, repo, pr_num):
    """
    Fetch labels from PR, get cherry-pick tags and return target branches

    :return: list of target branch names
    """

    # fetch run first, get workflow id from there to get workflow runs
    r = s.get(f'https://api.github.com/repos/{repo}/pulls/{pr_num}')
    r.raise_for_status()
    j = r.json()

    targets = []
    for label in j['labels']:
        name = label['name']
        if not name.startswith(CHERRY_PICK_TAG_PREFIX):
            continue
        targets.append(name.replace(CHERRY_PICK_TAG_PREFIX, ''))

    return targets


def get_pr_commits(s, repo, pr_num):
    """
    Fetch commits for

    :return: list of target branch names
    """

    # fetch run first, get workflow id from there to get workflow runs
    r = s.get(f'https://api.github.com/repos/{repo}/pulls/{pr_num}/commits')
    r.raise_for_status()
    j = r.json()

    return [c['sha'] for c in j]


def post_pr_comment(s, repo, pr_num, info=None, warnings=None):
    run_id = os.environ['WORKFLOW_RUN_ID']

    if not info:
        info = ['N/A']
    if not warnings:
        warnings = ['N/A']

    formatted_info = '\n'.join(f'- {e}' for e in info)
    formatted_warnings = '\n'.join(f'- {e}' for e in warnings)
    comment = PR_MESSAGE.format(formatted_info, formatted_warnings, repo, run_id)

    r = s.post(f'https://api.github.com/repos/{repo}/issues/{pr_num}/comments',
               json=dict(body=comment))
    r.raise_for_status()


def cherry_pick_to(target, commits):
    try:
        print(f'Checking out branch "{target}"...')
        _ = check_output(['git', 'checkout', '-f', target], stderr=STDOUT)
    except CalledProcessError as e:
        err = e.output.decode('utf-8')
        print('⚠ Checking out failed:', err)
        return err

    try:
        print(f'Picking commits to "{target}"...')
        _ = check_output(['git', 'cherry-pick', *commits], stderr=STDOUT)
        return ''
    except CalledProcessError as e:
        err = e.output.decode('utf-8')
        print('⚠ Cherry-picking failed:', err)

        # Also try to abort cherry-pick, just in case
        try:
            check_output(['git', 'cherry-pick', '--abort'], stderr=STDOUT)
        except CalledProcessError as _e:
            print(f'⚠ Aborting cherry-pick failed: {_e!r}')

        return err


def push_branches(branches):
    try:
        _ = check_output(['git', 'push', 'origin', *branches], stderr=STDOUT)
    except CalledProcessError as e:
        err = e.output.decode('utf-8')
        print('⚠ Pushing branches failed:', err)
        return err


def main():
    # comment contents
    warnings = []
    info = []

    session = requests.session()
    session.headers['Accept'] = 'application/vnd.github+json'
    session.headers['Authorization'] = f'Bearer {os.environ["GITHUB_TOKEN"]}'
    repo = os.environ['REPOSITORY']
    pr_number = os.environ['PR_NUMBER']

    try:
        targets = get_pr_targets(session, repo, pr_number)
    except Exception as e:
        print(f'⚠ Fetching PR tags failed: {e!r}')
        return 1

    if not targets:
        print(f'No cherry pick requested for PR {pr_number}')
        return 0

    # get the PR's commits to pick
    try:
        commits = get_pr_commits(session, repo, pr_number)
        if not commits:
            raise ValueError('No commits in PR?!')
    except Exception as e:
        print(f'⚠ Fetching PR tags failed: {e!r}')
        return 1

    successes = []
    for target in targets:
        if err := cherry_pick_to(target, commits):
            warnings.append(f'Failed to merge into `{target}`: `{err}`')
        else:
            info.append(f'Successfully merged into `{target}`')
            successes.append(target)

    if push_err := push_branches(successes):
        warnings.append(f'Failed to push: {push_err}')
    else:
        info.append(f'Successfully pushed all branches.')

    try:
        post_pr_comment(session, repo, pr_number, info, warnings)
    except Exception as e:
        print(f'⚠ Posting PR comment failed: {e!r}')

    # if no branches were successfully picked or pushing failed, also set an error status
    if not successes or push_err:
        return 1

    return 0


if __name__ == '__main__':
    sys.exit(main())
