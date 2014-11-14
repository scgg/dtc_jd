#include "plugin_operate.h"
#include "log.h"

int PluginGet::handle(std::string key, int keyType, TTC::Result& rst)
{
	int ret = 0;
	TTC::Server srv;
	srv.SetAddress("127.0.0.1","9899");
	srv.SetTableName(srv.GetServerTableName());
	srv.IntKey();
	srv.AddKey("key",TTC::KeyTypeInt);
	log_debug("ServerAddress=%s,TableName=%s",srv.GetServerAddress(),srv.GetServerTableName());
	srv.SetMTimeout(5000);
	TTC::GetRequest req(&srv);
	/* keyTpye: None=0, Signed=1, Unsigned=2, Float=3, String=4, Binary=5 */
	switch(keyType)
	{
		case 1:
		case 2:
			long long int lkey;
			lkey = atoi(key.c_str());
			ret = req.SetKey(lkey);
			break;
		case 3:
		case 4:
		case 5:
			ret = req.SetKey(key.c_str());
			break;
		default:
			break;
	}

	if(0 == ret)
		ret = req.Need("len");
	if(0 == ret)
		ret = req.Need("data");
    if(0 != ret)
    {
		log_error("get-req set key or need error: %d", ret);
		return -1;
    }
    ret = req.Execute(rst);
    if(0 != ret) {
    	return ret;
    }
    return ret;
}




