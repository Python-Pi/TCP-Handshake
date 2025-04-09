#!/bin/bash

# Initialize git repository
git init

# Add all files
git add .

# Create initial commit
git commit -m "Initial commit for TCP Handshake implementation"

# Set the remote repository
git remote add origin https://github.com/Python-Pi/TCP-Handshake.git

# Rename the branch to main
git branch -M main

# Push to the remote repository
git push -u origin main

echo "Git repository initialized and pushed to GitHub!"