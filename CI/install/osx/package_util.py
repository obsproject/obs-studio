def cmd(cmd):
    import subprocess
    import shlex
    return subprocess.check_output(shlex.split(cmd)).rstrip('\r\n')

def get_tag_info(tag):
    rev = cmd('git rev-parse {0}'.format(latest_tag))
    anno = cmd('git cat-file -p {0}'.format(rev))
    tag_info = []
    for i, v in enumerate(anno.splitlines()):
        if i <= 4:
            continue
        tag_info.append(v.lstrip())

    return tag_info

def gen_html(github_user, latest_tag):

    url = 'https://github.com/{0}/obs-studio/commit/%H'.format(github_user)

    with open('readme.html', 'w') as f:
        f.write("<html><body>")
        log_cmd = """git log {0}...HEAD --pretty=format:'<li>&bull; <a href="{1}">(view)</a> %s</li>'"""
        log_res = cmd(log_cmd.format(latest_tag, url))
        if len(log_res.splitlines()):
            f.write('<p>Changes since {0}: (Newest to oldest)</p>'.format(latest_tag))
            f.write(log_res)

        ul = False
        f.write('<p>')
        import re

        for l in get_tag_info(latest_tag):
            if not len(l):
                continue
            if l.startswith('*'):
                ul = True
                if not ul:
                    f.write('<ul>')
                f.write('<li>&bull; {0}</li>'.format(re.sub(r'^(\s*)?[*](\s*)?', '', l)))
            else:
                if ul:
                    f.write('</ul><p/>')
                ul = False
                f.write('<p>{0}</p>'.format(l))
        if ul:
            f.write('</ul>')
        f.write('</p></body></html>')

    cmd('textutil -convert rtf readme.html -output readme.rtf')
    cmd("""sed -i '' 's/Times-Roman/Verdana/g' readme.rtf""")

def save_manifest(latest_tag, user, jenkins_build, branch, stable):
    log = cmd('git log --pretty=oneline {0}...HEAD'.format(latest_tag))
    manifest = {}
    manifest['commits'] = []
    for v in log.splitlines():
        manifest['commits'].append(v)
    manifest['tag'] = {
        'name': latest_tag,
        'description': get_tag_info(latest_tag)
    }

    manifest['version'] = cmd('git rev-list HEAD --count')
    manifest['sha1'] = cmd('git rev-parse HEAD')
    manifest['jenkins_build'] = jenkins_build

    manifest['user'] = user
    manifest['branch'] = branch
    manifest['stable'] = stable

    import cPickle
    with open('manifest', 'w') as f:
        cPickle.dump(manifest, f)

def prepare_pkg(project, package_id):
    cmd('packagesutil --file "{0}" set package-1 identifier {1}'.format(project, package_id))
    cmd('packagesutil --file "{0}" set package-1 version {1}'.format(project, '1.0'))


import argparse
parser = argparse.ArgumentParser(description='obs-studio package util')
parser.add_argument('-u', '--user', dest='user', default='jp9000')
parser.add_argument('-p', '--package-id', dest='package_id', default='org.obsproject.pkg.obs-studio')
parser.add_argument('-f', '--project-file', dest='project', default='OBS.pkgproj')
parser.add_argument('-j', '--jenkins-build', dest='jenkins_build', default='0')
parser.add_argument('-b', '--branch', dest='branch', default='master')
parser.add_argument('-s', '--stable', dest='stable', required=False, action='store_true', default=False)
args = parser.parse_args()

latest_tag = cmd('git describe --tags --abbrev=0')
gen_html(args.user, latest_tag)
prepare_pkg(args.project, args.package_id)
save_manifest(latest_tag, args.user, args.jenkins_build, args.branch, args.stable)
