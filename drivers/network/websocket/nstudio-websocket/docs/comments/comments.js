const fs = require('fs');
const path = require('path');
const glob = require('glob');
const parseComments = require('parse-comments');
const config = require('./config.json');

const parseFiles = files => {
    let response = [];
    files.forEach(file => {
        const f = fs.readFileSync(file, 'utf8').toString();
        response = response.concat(parseComments(f));
    });

    return response;
};

const files = glob.sync(config.srcGlob);
const comments = parseFiles(files);

if (!fs.existsSync(config.outDirectory)){
    fs.mkdirSync(config.outDirectory);
}

fs.writeFileSync(path.join(config.outDirectory, 'comments.json'), JSON.stringify(comments, null, 2));
