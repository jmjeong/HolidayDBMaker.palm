## Makefile for subway

upload: Release/HolidayDBMaker.prc
	zip -r HolidayDBMaker.zip Release/HolidayDBMaker.prc Release/readme.txt
	scp HolidayDBMaker.zip jmjeong.com:~/wikix/myfile/HolidayDBMaker

clean:
	-rm -f *.[oa] $(TARGET) *.bin bin.res *.grc Makefile.bak

veryclean: clean
	-rm -f $(TARGET).prc pilot.ram pilot.scratch

