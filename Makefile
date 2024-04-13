
.PHONY : default clean debug

default:
	cd src && $(MAKE) $@

clean:
	cd src && $(MAKE) $@

debug:
	cd src && $(MAKE) $@

# test:
# 	cd src && $(MAKE) $@
# .PHONY: test

# valgrind:
# 	$(MAKE) OPTIMIZATION="-O0" MALLOC="libc"

# helgrind:
# 	$(MAKE) OPTIMIZATION="-O0" MALLOC="libc" CFLAGS="-D__ATOMIC_VAR_FORCE_SYNC_MACROS"

# lcov:
# 	$(MAKE) gcov
# 	@./runtest || true
# 	@lcov -c -o src/all.info -d src/
# 	@lcov --extract src/all.info '*/src/reply_order*' '*/src/reply_order*' '*/src/protocol*' '*/src/slowlog*' '*/src/config*' '*/src/http*' '*/src/cluster*' '*/src/lion*' '*/src/proxy*' '*/src/zk_client*' -o src/proxy.info
# 	@genhtml --legend -o src/lcov-html src/proxy.info
# .PHONY: lcov

# gcov:
# 	$(MAKE) OPTIMIZATION="-O0" CFLAGS="-fprofile-arcs -ftest-coverage -DCOVERAGE_TEST" LDFLAGS="-fprofile-arcs -ftest-coverage"

