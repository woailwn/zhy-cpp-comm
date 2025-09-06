include config.mk
all:
	@for dir in $(BUILD_DIR); \
		do \
			make -C $$dir; \
		done

clean:
#*.gch 文件就是 GCC 的 预编译头文件,属于编译过程的中间产物
	rm -rf app/link_obj app/dep zhy_nginx
	rm -rf signal/*.gch app/*.gch

