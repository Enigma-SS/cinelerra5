#!/usr/bin/perl

# Helper script for context help in Cinelerra
# Calling: ContextManual.pl "<help keyphrase>"
# Searches the requested key in the following order:
# 1) manual Contents
# 2) manual Index
# 3) all manual pages via grep
# 4) FFmpeg or Ladspa plugins
# The first item found is shown via the default web browser
# If nothing found, the Contents itself is shown
# On empty keyphrase do nothing
# The special keyphrase "TOC" shows Contents, "IDX" shows Index
# The keyphrase starting with "FILE:" shows the file named after colon

# Several important definitions

# Web browser executable
$cin_browser = $ENV{'CIN_BROWSER'};
# a likely default browser
$cin_browser = 'firefox' if $cin_browser eq '';
# another possible browser
#$cin_browser = 'xdg-open' if $cin_browser eq '';
# a fake browser for debugging
#$cin_browser = 'echo';

# The node with the manual contents
$contents_node = 'Contents.html';

# The node with the manual index
$index_node = 'Index.html';

# Several special plugin names necessary to rewrite
%rewrite = (
  # Rendered effects and transitions are not segmented in the Contents
  "CD Ripper"           => "Rendered Audio Effects",
  "Normalize"           => "Rendered Audio Effects",
  "Resample"            => "Rendered Audio Effects",
  "Time stretch"        => "Rendered Audio Effects",

  "720 to 480"          => "Rendered Video Effects",
  "Reframe"             => "Rendered Video Effects",

  # Audio transitions are segmented in the Index
#  "Crossfade"           => "Audio Transitions",

  # Video transitions are segmented in the Index
#  "BandSlide"           => "Video Transitions",
#  "BandWipe"            => "Video Transitions",
#  "Dissolve"            => "Video Transitions",
#  "Flash"               => "Video Transitions",
#  "IrisSquare"          => "Video Transitions",
#  "Shape Wipe"          => "Video Transitions",
#  "Slide"               => "Video Transitions",
#  "Wipe"                => "Video Transitions",
#  "Zoom"                => "Video Transitions",

  # Several not properly matched names
  "AgingTV"             => "Aging TV",
  "Brightness/Contrast" => "Brightness\\/Contrast",
  "Chroma key (HSV)"    => "Chroma Key \\(HSV\\)",
  "Crop & Position"     => "Crop &amp; Position",
  "FindObj"             => "Find Object",
  "RGB - 601"           => "RGB-601",
  "ShiftInterlace"      => "Shift Interlace",
  "Cinelerra: Scopes"   => "Videoscope"
  );

# Cinelerra installation path
$cin_dat = $ENV{'CIN_DAT'};
$cin_dat = '.' if $cin_dat eq '';

# Cinelerra HTML manual must reside here
$cin_man = "$cin_dat/doc/CinelerraGG_Manual";
$contents = $cin_man.'/'.$contents_node;
$index = $cin_man.'/'.$index_node;
#print "ContextManual: using contents $contents\n";

# 1st argument is the requested key
$help_key = $ARGV[0];
#print "ContextManual: request=$help_key\n";
# Do nothing if no key requested
exit 0 if $help_key eq '';
# Show contents on this special request
if ($help_key eq 'TOC')
{
  system "$cin_browser \"file://$contents\" &";
  exit 0;
}
# Show index on this special request
if ($help_key eq 'IDX')
{
  system "$cin_browser \"file://$index\" &";
  exit 0;
}
# Show the named file on this special request
if ($help_key =~ /^FILE:/)
{
  $help_key =~ s/^FILE://;
  $help_key = $cin_man.'/'.$help_key;
  system "$cin_browser \"file://$help_key\" &";
  exit 0;
}

$help_key = $rewrite{$help_key} if exists $rewrite{$help_key};
# Do nothing if no key requested
exit 0 if $help_key eq '';
# Show contents on this special request
if ($help_key eq 'TOC')
{
  system "$cin_browser \"file://$contents\" &";
  exit 0;
}
# Show index on this special request
if ($help_key eq 'IDX')
{
  system "$cin_browser \"file://$index\" &";
  exit 0;
}
# Show the named file on this special request
if ($help_key =~ /^FILE:/)
{
  $help_key =~ s/^FILE://;
  $help_key = $cin_man.'/'.$help_key;
  system "$cin_browser \"file://$help_key\" &";
  exit 0;
}

# Now try searching...
open CONTENTS, $contents or die "Cannot open $contents: $!";
$node = '';
# First search contents for the exact key
while (<CONTENTS>)
{
  chomp;
  last if ($node) = /^\s*HREF=\"(.+?\.html)\">\s*$help_key\s*<\/A>$/;
}
# If not found, search contents for an approximate key
if ($node eq '')
{
  seek CONTENTS, 0, 0;
  while (<CONTENTS>)
  {
    chomp;
    last if ($node) = /^\s*HREF=\"(.+?\.html)\">.*?$help_key.*?<\/A>$/i;
  }
}

# If not found, search index for the exact key
if ($node eq '')
{
  open INDEX, $index or die "Cannot open $index: $!";
  while (<INDEX>)
  {
    chomp;
    # Cut off anchor: xdg-open does not like it
    last if ($node) = /<A\s+HREF=\"(.+?\.html)(?:#\d+)?\">\s*$help_key\s*<\/A>$/;
    # Retain anchor
#    last if ($node) = /<A\s+HREF=\"(.+?\.html(?:#\d+)?)\">\s*$help_key\s*<\/A>$/;
  }
  close INDEX;
}
# If not found, search index for an approximate key
if ($node eq '')
{
  open INDEX, $index or die "Cannot open $index: $!";
  while (<INDEX>)
  {
    chomp;
    # Cut off anchor: xdg-open does not like it
    last if ($node) = /<A\s+HREF=\"(.+?\.html)(?:#\d+)?\">.*?$help_key.*?<\/A>$/i;
    # Retain anchor
#    last if ($node) = /<A\s+HREF=\"(.+?\.html(?:#\d+)?)\">.*?$help_key.*?<\/A>$/i;
  }
  close INDEX;
}

# If not found, grep manual for exact key instance
if ($node eq '')
{
  $_ = `grep -l \"$help_key\" $cin_dat/doc/CinelerraGG_Manual/*.html`;
  ($node) = split;
}
# If not found, grep manual for case insensitive key instance
if ($node eq '')
{
  $_ = `grep -il \"$help_key\" $cin_dat/doc/CinelerraGG_Manual/*.html`;
  ($node) = split;
}

if ($node eq '')
{
  if ($help_key =~ /^F_/)
  { # If not found, search contents for FFmpeg plugins
    $help_key = 'FFmpeg Audio and Video Plugins';
    seek CONTENTS, 0, 0;
    while (<CONTENTS>)
    {
      chomp;
      last if ($node) = /^\s*HREF=\"(.+?\.html)\">\s*$help_key\s*<\/A>$/;
    }
  }
  elsif ($help_key =~ /^L_/)
  { # If not found, search contents for LADSPA plugins
    $help_key = 'Audio Ladspa Effects';
    seek CONTENTS, 0, 0;
    while (<CONTENTS>)
    {
      chomp;
      last if ($node) = /^\s*HREF=\"(.+?\.html)\">\s*$help_key\s*<\/A>$/;
    }
  }
}

close CONTENTS;

# If still nothing found, show contents
$node = $contents_node if $node eq '';
$node = $cin_man.'/'.$node unless $node =~ /\//;
#print "ContextManual: found $node\n";

# Call browser to show the proposed HTML file
system "$cin_browser \"file://$node\" &";

# And immediately return to the caller
exit 0;
