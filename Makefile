include config.mk
all:
	@for dir in $(BUILD_DIR); \
		do \
			make -C $$dir; \
		done

clean:
#*.gch �ļ����� GCC �� Ԥ����ͷ�ļ�,���ڱ�����̵��м����
	rm -rf app/link_obj app/dep zhy_nginx
	rm -rf signal/*.gch app/*.gch

