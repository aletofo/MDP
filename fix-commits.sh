#!/bin/bash
python3 git-filter-repo --force --name-callback'
if commit.author_name == "ALESSIO TOFONI" and commit.author_email == "273824@studenti.unimore.it":
    commit.author_name = "aletofo"
    commit.author_email = "atofoni6@gmail.com"
return commit
'

