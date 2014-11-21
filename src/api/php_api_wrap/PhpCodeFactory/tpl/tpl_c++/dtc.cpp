#include "dtc_{TableName}.h"
using namespace DTC;

DTC{TableName}::DTC{TableName}(){
	init();
}

DTC{TableName}::~DTC{TableName}(){

}

void DTC{TableName}::clearErr(){
	errCode = 0;
	errMsg  = "";
	errFrom = "";
	dtcCode = 0;
}

void DTC{TableName}::init(){
	int ret = 0;
	clearErr();
	dtckey = dtcKey;
	dtcServer.SetTimeout(timeout);
	ret = dtcServer.SetTableName(tablename.c_str());
	if(0 != ret){
		errCode = ret;
		errMsg = dtcServer.ErrorMessage();
		dtcServer.Close();
	}
	ret = dtcServer.SetAddress(ip.c_str(), port.c_str());
	if(0 != ret){
		errCode = ret;
		errMsg = dtcServer.ErrorMessage();
		dtcServer.Close();
	}
	dtcServer.SetAccessKey(token.c_str());
	int keyflag = 0;
	int itmp = sizeof(fieldcfg)/(sizeof(fieldcfg[0][0])*4);
	for(int i=0; i< itmp;i++){
		if(fieldcfg[i][0] == dtcKey){
			keyflag = i;
			break;
		}
	}
	keyType = atoi(fieldcfg[keyflag][1].c_str());
	keyIndex = keyflag;
	if(keyType == Signed || keyType == Unsigned){
		ret = dtcServer.IntKey();
		if(0 != ret){
			errCode = ret;
			errMsg = dtcServer.ErrorMessage();
			dtcServer.Close();
		}
	}else if(keyType == String){
		ret = dtcServer.StringKey();
		if(0 != ret){
			errCode = ret;
			errMsg = dtcServer.ErrorMessage();
			dtcServer.Close();
		}
	}else if(keyType == Binary){
		ret = dtcServer.BinaryKey();
		if(0 != ret){
			errCode = ret;
			errMsg = dtcServer.ErrorMessage();
			dtcServer.Close();
		}
	}else{
		errCode = E_INVALID_KEY_TYPE;
		errMsg = "invalid key type";
		dtcServer.Close();
	}
}
/*
 * @describe 清理缓存
 *
 * @param key dtc key 字段值(fieldvalue)
 *
 * @return bool
 */
bool DTC{TableName}::purge(string key){
	int ret = 0;
	clearErr();
	GetRequest purReq(&dtcServer);
	if(checkKey(key) == false)
		return false;
	if(keyType == Signed || keyType == Unsigned){
		ret = purReq.SetKey(atoi(key.c_str()));
	}else if(keyType == String){
		ret = purReq.SetKey(key.c_str());
	}else if(keyType == Binary){
		ret = purReq.SetKey(key.c_str(), key.size());
	}else{
		errCode = E_INVALID_KEY_TYPE;
		errMsg  = "purge request failed: bad keyType!";
		return false;
	}
	if(0 != ret){
		errCode = ret;
		errMsg  = "##dtc## purge setkey fail!";
		return false;
	}
	Result rst;
	ret = purReq.Execute(rst);
	if(0 != ret){
		dtcCode = rst.ResultCode();
		errCode = ret;
		errMsg = rst.ErrorMessage();
		errFrom = rst.ErrorFrom();
		return false;
	}
	affectRows = rst.AffectedRows();
	return true;
}
/*
 * @describe 删除一条数据库记录
 *
 * @param key    dtc key 字段值(fieldvalue)
 */
bool DTC{TableName}::delet(string key){
	int ret = 0;
	clearErr();
	DeleteRequest delReq(&dtcServer);
	if(checkKey(key) == false)
		return false;
	if(keyType == Signed || keyType == Unsigned){
		ret = delReq.SetKey(atoi(key.c_str()));
	}else if(keyType == String){
		ret = delReq.SetKey(key.c_str());
	}else if(keyType == Binary){
		ret = delReq.SetKey(key.c_str(), key.size());
	}else{
		errCode = E_INVALID_KEY_TYPE;
		errMsg  = "purge request failed: bad keyType!";
		return false;
	}
	if(0 != ret){
		errCode = ret;
		errMsg  = "##dtc## purge setkey fail!";
		return false;
	}
	Result rst;
	ret = delReq.Execute(rst);
	if(0 != ret){
		dtcCode = rst.ResultCode();
		errCode = ret;
		errMsg = rst.ErrorMessage();
		errFrom = rst.ErrorFrom();
		return false;
	}
	affectRows = rst.AffectedRows();
	return true;
}
/*
 * 获得DTC记录
 * @param   string  key, dtc key 字段值(fieldvalue)
 * @param   vector<Field>   need, 可选参数, 默认返回所有列，也可以指明需要返回的列
 * @param 	vector<Field>	resultSet, 返回结果集
 * @atention 当指定need 条件时，必须传入Field类对象中的fieldName以及fieldType两个参数
 */
bool DTC{TableName}::get(string key, vector<Field>& need, vector<Field>& resultSet){
	int ret = 0;
	clearErr();
	GetRequest getReq(&dtcServer);
	if(checkKey(key) == false)
		return false;
	if(keyType == Signed || keyType == Unsigned){
		ret = getReq.SetKey(atoi(key.c_str()));
	}else if(keyType == String){
		ret = getReq.SetKey(key.c_str());
	}else if(keyType == Binary){
		ret = getReq.SetKey(key.c_str(), key.size());
	}else{
		errCode = E_INVALID_KEY_TYPE;
		errMsg  = "update request failed: bad keyType!";
		return false;
	}
	if(0 != ret){
		errCode = ret;
		errMsg  = "##dtc## update setkey fail!";
		return false;
	}
	if(need.size() == 0){
		int itmp = sizeof(fieldcfg)/(sizeof(fieldcfg[0][0])*4);
		for(int i=0; i< itmp; ++i){
			if(fieldcfg[i][0] == dtckey)
				continue;
			ret = getReq.Need(fieldcfg[i][0].c_str());
			if(0 != ret){
				errCode = ret;
				errMsg  = "##dtc## get Need error without need list!";
				return false;
			}
		}
	}else{
		for(vector<Field>::iterator iter = need.begin(); iter!= need.end(); ++iter){
			Field fd = *iter;
			if(fd.fieldName == ""){
				errCode = E_EMPTY_FIELDNAME;
				errMsg  = "##dtc## get Need error---empty fieldname!";
				return false;
			}
			if(fd.fieldName == dtckey)
				continue;
			ret = getReq.Need(fd.fieldName.c_str());
			if(0 != ret){
				errCode = ret;
				errMsg  = "##dtc## get Need error with need list!";
				return false;
			}
		}
	}

	// 调用dtc api 执行查询操作
	Result rst;
	ret = getReq.Execute(rst);
	if(0 != ret){
		dtcCode = rst.ResultCode();
		errCode = ret;
		errMsg = rst.ErrorMessage();
		errFrom = rst.ErrorFrom();
		return false;
	}
	affectRows = rst.AffectedRows();
	int rowCount = rst.NumRows();
	// 拼装结果集
	for (int i = 0 ; i < rowCount; ++i){
		ret = rst.FetchRow();
		if(ret < 0){
			errCode = ret;
			errMsg = rst.ErrorMessage();
			errFrom = rst.ErrorFrom();
			return false;
		}
		// 不带need 条件，返回所有满足key条件的结果集
		if(need.size() == 0){
			int itmp = sizeof(fieldcfg)/(sizeof(fieldcfg[0][0])*4);
			for(int j=0; j< itmp; ++j){
				if(fieldcfg[j][0] == dtckey)
					continue;
				Field resultField;
				stringstream sstmp;
				string tmp;
				switch (atoi(fieldcfg[j][1].c_str())){
				case Signed:
					resultField.fieldType = Signed;
					resultField.fieldName = fieldcfg[j][0];

					sstmp << rst.IntValue(fieldcfg[j][0].c_str());
					sstmp >> tmp;
					resultField.fieldValue = tmp;
					resultSet.push_back(resultField);
					break;
				case Unsigned:
					resultField.fieldType = Unsigned;
					resultField.fieldName = fieldcfg[j][0];

					sstmp << rst.IntValue(fieldcfg[j][0].c_str());
					sstmp >> tmp;
					resultField.fieldValue = tmp;
					resultSet.push_back(resultField);
					break;
				case Float:
					resultField.fieldType = Float;
					resultField.fieldName = fieldcfg[j][0];

					sstmp << rst.FloatValue(fieldcfg[j][0].c_str());
					sstmp >> tmp;
					resultField.fieldValue = tmp;
					resultSet.push_back(resultField);
					break;
				case String:
					resultField.fieldType = String;
					resultField.fieldName = fieldcfg[j][0];
					tmp = rst.StringValue(fieldcfg[j][0].c_str());
					resultField.fieldValue = tmp;
					resultSet.push_back(resultField);
					break;
				case Binary:
					resultField.fieldType = Binary;
					resultField.fieldName = fieldcfg[j][0];
					tmp = rst.BinaryValue(fieldcfg[j][0].c_str());
					resultField.fieldValue = tmp;
					resultSet.push_back(resultField);
					break;
				default:
					errCode = E_INVALID_FIELD_TYPE;
					errMsg  = "##dtc## get invalid feildType without need condition!";
					return false;
				}// end switch
			}// end for
		}else{
			for(vector<Field>::iterator it = need.begin(); it!= need.end(); ++it){
				Field field = *it;
				Field resultField;
				if(field.fieldType == None){
					errCode = E_INVALID_FIELD_TYPE;
					errMsg  = "##dtc## get invalid fieldType, fieldType is 0!";
					return false;
				}
				if(field.fieldName == dtckey)
					continue;
				stringstream sstmp;
				string tmp;
				switch(field.fieldType){
				case Signed:
					resultField.fieldType = Signed;
					resultField.fieldName = field.fieldName;

					sstmp << rst.IntValue(field.fieldName.c_str());
					sstmp >> tmp;
					resultField.fieldValue = tmp;
					resultSet.push_back(resultField);
					break;
				case Unsigned:
					resultField.fieldType = Unsigned;
					resultField.fieldName = field.fieldName;

					sstmp << rst.IntValue(field.fieldName.c_str());
					sstmp >> tmp;
					resultField.fieldValue = tmp;
					resultSet.push_back(resultField);
					break;
				case Float:
					resultField.fieldType = Float;
					resultField.fieldName = field.fieldName;

					sstmp << rst.FloatValue(field.fieldName.c_str());
					sstmp >> tmp;
					resultField.fieldValue = tmp;
					resultSet.push_back(resultField);
					break;
				case String:
					resultField.fieldType = String;
					resultField.fieldName = field.fieldName;
					tmp = rst.StringValue(field.fieldName.c_str());
					resultField.fieldValue = tmp;
					resultSet.push_back(resultField);
					break;
				case Binary:
					resultField.fieldType = Binary;
					resultField.fieldName = field.fieldName;
					tmp = rst.BinaryValue(field.fieldName.c_str());
					resultField.fieldValue = tmp;
					resultSet.push_back(resultField);
					break;
				default:
					errCode = E_INVALID_FIELD_TYPE;
					errMsg  = "##dtc## get invalid feildType with need condition!";
					return false;
				}// end switch
			}// end for
		}// end else

	}
	
	return true;
}
/*
 * @describe: 更新一条数据库记录
 * @param:  key dtc key 字段值(fieldvalue)
 * @param vector<Field> fieldList 需要更新的字段集，每个子集Field必须指定 fieldName,fieldType以及fieldValue三个参数
 */
bool DTC{TableName}::update(string key, vector<Field>& fieldList){
	int ret = 0;
	clearErr();
	UpdateRequest updateReq(&dtcServer);
	if(checkKey(key) == false)
		return false;
	if (check(fieldList) == false)
		return false;
	if(keyType == Signed || keyType == Unsigned){
		ret = updateReq.SetKey(atoi(key.c_str()));
	}else if(keyType == String){
		ret = updateReq.SetKey(key.c_str());
	}else if(keyType == Binary){
		ret = updateReq.SetKey(key.c_str(), key.size());
	}else{
		errCode = E_INVALID_KEY_TYPE;
		errMsg  = "update request failed: bad keyType!";
		return false;
	}
	if(0 != ret){
		errCode = ret;
		errMsg  = "##dtc## update setkey fail!";
		return false;
	}
	for(vector<Field>::iterator iter = fieldList.begin(); iter != fieldList.end(); ++iter){
		Field fd = *iter;
		if(fd.fieldName == "")
		{
			continue;
		}
		if(fd.fieldName == dtckey)
		{
			continue;
		}
		stringstream sstmp;
		switch(fd.fieldType){
		case Signed:
		case Unsigned:
			sstmp << fd.fieldValue;
			int itmp;
			sstmp >> itmp;
			ret = updateReq.Set(fd.fieldName.c_str(), itmp);
			break;
		case Float:
			sstmp << fd.fieldValue;
			double dtmp;
			sstmp >> dtmp;
			ret = updateReq.Set(fd.fieldName.c_str(), dtmp);
			break;
		case String:
			ret = updateReq.Set(fd.fieldName.c_str(), fd.fieldValue.c_str());
			break;
		case Binary:
			ret = updateReq.Set(fd.fieldName.c_str(), fd.fieldValue.c_str());
			break;
		default:
			break;
		}
	}
	Result rst;
	ret = updateReq.Execute(rst);
	if(0 != ret){
		dtcCode = rst.ResultCode();
		errCode = ret;
		errMsg = rst.ErrorMessage();
		errFrom = rst.ErrorFrom();
		return false;
	}
	affectRows = rst.AffectedRows();
	return true;
}
/*
 * @describe 增加一条数据库记录
 *
 * @param:  key dtc key 字段值(fieldvalue)
 * @param List<Field> fieldList 需要插入的字段集，每个子集Field必须指定 fieldName以及fieldType,fieldValue三个参数
 */
bool DTC{TableName}::insert(string key, vector<Field>& fieldList){
	int ret = 0;
	clearErr();
	InsertRequest insrtReq(&dtcServer);
	if(checkKey(key) == false)
		return false;
	if (check(fieldList) == false)
		return false;
	if(keyType == Signed || keyType == Unsigned){
		ret = insrtReq.SetKey(atoi(key.c_str()));
	}else if(keyType == String){
		ret = insrtReq.SetKey(key.c_str());
	}else if(keyType == Binary){
		ret = insrtReq.SetKey(key.c_str(), key.size());
	}else{
		errCode = E_INVALID_KEY_TYPE;
		errMsg  = "insert request failed: bad keyType!";
		return false;
	}
	if(0 != ret){
		errCode = ret;
		errMsg  = "##dtc## insert setkey fail!";
		return false;
	}
	for(vector<Field>::iterator iter = fieldList.begin(); iter != fieldList.end(); ++iter){
		Field fd = *iter;
		stringstream sstmp;
		switch(fd.fieldType){
		case Signed:
		case Unsigned:
		{
			sstmp << fd.fieldValue;
			int itmp;
			sstmp >> itmp;
			ret = insrtReq.Set(fd.fieldName.c_str(), itmp);
			break;
		}
		case Float:
		{
			sstmp << fd.fieldValue;
			double dtmp;
			sstmp >> dtmp;
			ret = insrtReq.Set(fd.fieldName.c_str(), dtmp);
			break;
		}
		case String:
			ret = insrtReq.Set(fd.fieldName.c_str(), fd.fieldValue.c_str());
			break;
		case Binary:
			ret = insrtReq.Set(fd.fieldName.c_str(), fd.fieldValue.c_str());
			break;
		default:
			break;
		}
	}
	Result rst;
	ret = insrtReq.Execute(rst);
	if(0 != ret){
		dtcCode = rst.ResultCode();
		errCode = ret;
		errMsg = rst.ErrorMessage();
		errFrom = rst.ErrorFrom();
		return false;
	}
	affectRows = rst.AffectedRows();
	return true;
}

/*
 * @describe 返回影响条数
 * @return int AffectedRows
 */
int DTC{TableName}::getAffectRows(){
	return affectRows;
}

/*
 * @describe 返回影响条数
 * @return int AffectedRows
 */
bool DTC{TableName}::checkKey(string key){
	clearErr();
	switch(keyType){
	case Signed:
	case Unsigned:
	{
		int i = atoi(key.c_str());
		if(i < atoi(fieldcfg[keyIndex][2].c_str())
				|| i > atoi(fieldcfg[keyIndex][3].c_str())){
			errCode = E_INVALID_FIELD_VALUE;
			errMsg  = "##dtc## key error";
			return false;
		}
		break;
	}
	case String:
	case Binary:
	{
		int len = key.size();
		if (len < atoi(fieldcfg[keyIndex][2].c_str())
				|| len > atoi(fieldcfg[keyIndex][3].c_str())){
			errCode = E_INVALID_FIELD_VALUE;
			errMsg  = "##dtc## key error";
			return false;
		}
		break;
	}
	default:
		errCode = E_INVALID_KEY_TYPE;
		errMsg  = "##dtc## invalid keytype! ";
		return false;
	}
	return true;
}
/*
 * @describe 检测数据是否合法
 * @param fieldlist   vector<Field>类型，包括所有的字段信息
 */
bool DTC{TableName}::check(vector<Field>& fieldList){
	clearErr();
	for(vector<Field>::iterator iter = fieldList.begin(); iter!= fieldList.end(); ++iter){
	int i = 0;
	Field fd = *iter;
	if (fd.fieldName == "")
		continue;
	/* 获取该字段在DTCConfig.fieldcfg 数组中的 index */
	i = GetFlag(fd.fieldName);
	stringstream sstmp;
	if(i < 0){
		errCode = E_INVALID_FIELDNAME;
		sstmp.str("");
		sstmp.clear();
		sstmp << "invalid fieldname----can't find this field in dtcconfig! fieldname: %s" << fd.fieldName;
		sstmp >> errMsg;
		return false;
	}
	switch (fd.fieldType){
		case Signed:
		case Unsigned:
		case Float:
		{
			sstmp.str("");
			sstmp.clear();
			long value;
			sstmp << fd.fieldValue;
			sstmp >> value;

			sstmp.str("");
			sstmp.clear();
			long tmpMin;
			sstmp << fieldcfg[i][2];
			sstmp >> tmpMin;

			sstmp.str("");
			sstmp.clear();
			long tmpMax;
			sstmp << fieldcfg[i][3];
			sstmp >> tmpMax;
			if (value < tmpMin || value > tmpMax){
				errCode = E_INVALID_FIELD_VALUE;
				sstmp.str("");
				sstmp << "##dtc## int or float field error fieldvalue: %l" << value;
				sstmp >> errMsg;
				return false;
			}
			break;
		}
		case String:
		{
			int len = fd.fieldValue.size();
			if (len < atoi(fieldcfg[i][2].c_str()) || len > atoi(fieldcfg[i][3].c_str())){
				errCode = E_INVALID_FIELD_VALUE;
				sstmp.str("");
				sstmp.clear();
				sstmp << "##dtc## string field error fieldvalue: %s" << fd.fieldValue << ", fieldlength: %d" << len;
				sstmp >> errMsg;
				return false;
			}
			break;
		}
		case Binary:
		{
			int len1 = fd.fieldValue.size();
			if (len1 < atoi(fieldcfg[i][2].c_str()) || len1 > atoi(fieldcfg[i][3].c_str())){
				errCode = E_INVALID_FIELD_VALUE;
				sstmp.str("");
				sstmp.clear();
				sstmp << "##dtc## binary field error fieldvalue: %s" << fd.fieldValue << ", fieldlength: %d" << len1;
				sstmp >> errMsg;
				return false;
			}
			break;
		}
		default:
			errCode = E_INVALID_FIELD_TYPE;
			sstmp.str("");
			sstmp.clear();
			sstmp << "##dtc## invalid fieldtype: %d" << fd.fieldType;
			sstmp >> errMsg;
			return false;
		}// switch
	}//for
	return true;
}
/*
 * 获取该字段在DTCConfig.fieldcfg 数组中的 index
 */
int DTC{TableName}::GetFlag(string fieldName){
	int flag = 0;
	int i=0;
	int itmp = sizeof(fieldcfg)/(sizeof(fieldcfg[0][0])*4);
	for(; i<itmp; i++){
		if(fieldcfg[i][0] == fieldName){
			flag = i;
			break;
		}
	}
	if(i >= itmp)
		flag =-1;
	return flag;
}
