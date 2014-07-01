build:
	gcc -shared -fPIC -o rlm_metalive.rlm rlm_metalive.c

install:
	cp rlm_metalive.rlm ${DESTDIR}/usr/lib/rivendell/

clean:
	rm -f rlm_metalive.rlm
