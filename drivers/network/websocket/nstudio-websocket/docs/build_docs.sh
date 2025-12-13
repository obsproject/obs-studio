#!/bin/bash

cd comments
npm install
npm run comments

cd ../docs
python3 process_comments.py
python3 generate_md.py
