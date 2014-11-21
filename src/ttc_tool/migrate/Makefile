SVN_REVISION = $(shell test -d .git && (git svn info | grep "Last Changed Rev" | cut -d " " -f 4))

ifeq "$(SVN_REVISION)a" "a"
	SVN_REVISION = $(shell test -d .svn && (svn info | grep "Last Changed Rev" | cut -d " " -f 4))
endif

ifeq "$(SVN_REVISION)a" "a"
	SVN_REVISION = "(unknown)"
endif
PREFIX=../../
VPATH=. $(PREFIX)/cache $(PREFIX)common
INCLUDE = -I $(PREFIX)api/c_api -I $(PREFIX)common -I $(PREFIX)hot_backup -I $(PREFIX)stat
LIBTTC = $(PREFIX)api/c_api/libttc.so 
LIBCOMMON = -L$(PREFIX)common/ -lcommon 
CFLAG = -lz -DSVN_REVISION=$(SVN_REVISION) -g
TARGET= ttc_change_node_address ttc_get_cluster_state ttc_reload_cluster_nodes\
		ttc_set_node_state migrate_agent client
all:$(TARGET)

%.o:%.cc %.h
	g++ -o$@ -c  $< $(INCLUDE) $(CFLAG)
%:%.cc $(LIBTTC)
	g++ -o$@ $^ $(INCLUDE) $(CFLAG)

migrate_objs =  request.o key_checker.o migrate_unit.o migrate_main.o consistent_hash_selector.o\
		chash.o MarkupSTL.o proccess_command.o network.o log.o unix_lock.o file.o
migrate_agent: $(migrate_objs) $(LIBTTC) 
	g++ -o$@ $^ $(INCLUDE) $(CFLAG) 

TARNAME=migrate_tools
tar:all
	@rm -rf $(TARNAME)
	@mkdir $(TARNAME)
	@cp -a console $(TARNAME)
	@cp $(TARGET) migrate_agent.sh $(TARNAME)/console
	@mv $(TARNAME)/console/migrate_agent* $(TARNAME)/
	@cp README $(TARNAME)
	@tar zcf $(TARNAME).tgz $(TARNAME)
	@rm -rf $(TARNAME)

.PHONY:all clean

$(LIBTTC):
	@make -C $(PREFIX)api/c_api
$(LIBCOMMON):
	@make -C $(PREFIX)common
clean:
	@-rm -rf $(TARGET) *.o *.tgz
