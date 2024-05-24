test:
	cd tests && \
	bash clean.sh && \
	bash prepare.sh && \
	cd ../shell && \
	make clean && \
	make && \
	cd .. && \
	bash -c "setsid bash .test.sh > /dev/tty" && \
	echo "OK"
