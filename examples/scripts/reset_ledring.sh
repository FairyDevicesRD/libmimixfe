#!/bin/bash
for STTYF in 'stty -F' 'stty --file' 'stty -f' 'stty <' ;
do $STTYF /dev/tty >/dev/null 2>&1 && break ;
done ;
$STTYF /dev/ttyAMA0  hupcl ;
(sleep 0.1 2>/dev/null || sleep 1) ;
$STTYF /dev/ttyAMA0 -hupcl
