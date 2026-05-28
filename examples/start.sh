#!/bin/sh

xrdb -merge ~/.Xresources #Basically loads your Xresources settings into X's memory

# The '&' is mandatory. Launch your your programs of choice. These are just examples.
sxhkd &
dunst &
polybar &
