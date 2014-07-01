build:
	gcc -shared -fPIC -o rlm_metalive.rlm rlm_metalive.c

install:
	cp rlm_metalive.rlm ${DESTDIR}/usr/lib/rivendel

clean:
	rm rlm_metalive.rlm
