@echo off
rem Build Batch file for EX291 VDD
rem  By Peter Johnson, 2000
rem
rem $Id: run.bat,v 1.2 2000/12/18 06:28:00 pete Exp $
build -bcew
copy objfre\i386\ex291srv.dll ..\..\bin
