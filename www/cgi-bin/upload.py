#!/usr/bin/env python3

import warnings
warnings.filterwarnings("ignore", category=DeprecationWarning)

import cgi
import os
import sys

print("Content-Type: text/html\n")

form = cgi.FieldStorage()

if "file" in form:
    fileitem = form["file"]
    if fileitem.filename:
        try:
            os.makedirs("./www/uploads", exist_ok=True)
            filename = os.path.basename(fileitem.filename)
            filepath = os.path.join("./www/uploads", filename)
            with open(filepath, 'wb') as f:
                f.write(fileitem.file.read())
            print("<html><body><h1>File Uploaded Successfully</h1></body></html>")
        except Exception as e:
            print(f"<html><body><h1>Failed to upload file: {e}</h1></body></html>")
    else:
        print("<html><body><h1>No file selected</h1></body></html>")
else:
    print("<html><body><h1>No file field in form</h1></body></html>")
