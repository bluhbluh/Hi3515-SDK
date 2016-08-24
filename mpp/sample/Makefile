
.PHONY: default clean

default:
	@for x in `ls `; do if [ -d $$x ]; then cd $$x; make; cd ../; fi; done

clean:
	@for x in `ls `; do if [ -d $$x ]; then cd $$x; make clean; cd ../; fi; done

