## Makefile for subway

upload: Release/HDMaker.prc
	zip -r HDMaker.zip Release/HDMaker.prc Release/readme.txt Release/lunarlib.prc
	scp HDMaker.zip jmjeong@jmjeong.com:~/wikix/myfile/HolidayDBMaker

clean:
	-rm -f *.[oa] $(TARGET) *.bin bin.res *.grc Makefile.bak

veryclean: clean
	-rm -f $(TARGET).prc pilot.ram pilot.scratch

