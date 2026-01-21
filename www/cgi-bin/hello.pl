#!/usr/bin/perl

use strict;
use warnings;

print "Content-Type: text/html\n\n";
print <<HTML;
<!DOCTYPE html>
<html lang="fr">
<head>
  <meta charset="UTF-8">
  <title>Script Perl CGI</title>
</head>
<body>
  <h1>Bonjour depuis un script Perl CGI !</h1>
  <p>\n Go read/watch : The Hitchhiker's Guide to the Galaxy !</p>
</body>
</html>
HTML
