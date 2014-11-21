<?php
function parseConfig($cfg, &$tDtl)
{
	$a = file($cfg);
	$flag = false;
	$last_index = 0;
	foreach($a as $line => $content){
		$arr = explode('=', $content);
	
		if(trim($arr[0]) == "DbName"){
			$value = trim($arr[1]);
			$tDtl['DB_KEY'] = $value;
		}
		else if(trim($arr[0]) == "DbNum"){
			$value = substr(trim($arr[1]), 1, strlen(trim($arr[1]))-2);
			$value2 = explode(',', $value);
			$v1 = trim($value2[0]);
			$v2 = trim($value2[1]);
			$tDtl['DB_MIN_NUM'] = $v1;
			$tDtl['DB_MAX_NUM'] = $v2;
		}
		else if(trim($arr[0]) == "TableName"){
			$value = trim($arr[1]);
			$tDtl['TABLE_KEY'] = $value;
			$tDtl['TABLE_NAME'] = $value;
		}
		else if(trim($arr[0] == "KeyFieldCount")){
			$value = trim($arr[1]);
			$tDtl['PRKEY_NUM'] = $value;
		}
		else if(trim($arr[0]) == "MachineNum"){
			$value = trim($arr[1]);
			$tDtl['MACHINE_NUM'] = $value;
		}
		else if(trim($arr[0]) == "DbAddr"){
			$value = trim($arr[1]);
			$tDtl['DB_IP'] = $value;
		}
		else if(trim($arr[0]) == "DbUser"){
			$value = trim($arr[1]);
			$tDtl['DB_USER'] = $value;
		}
		else if(trim($arr[0]) == "DbPass"){
			$value = trim($arr[1]);
			$tDtl['DB_PASS'] = $value;
		}
		else if(trim($arr[0]) == "Deploy"){
			$value = trim($arr[1]);
			$tDtl['DEPLOY'] = $value;
		}
		else if(trim($arr[0]) == "Procs"){
			$value = trim($arr[1]);
			$tDtl['PROCS_NUM'] = $value;
		}
		else if(trim($arr[0]) == "WriteProcs"){
			$value = trim($arr[1]);
			$tDtl['WRITE_PROCS_NUM'] = $value;
		}
		else if(trim($arr[0]) == "CommitProcs"){
			$value = trim($arr[1]);
			$tDtl['COMMIT_PROCS_NUM'] = $value;
		}
		else if(trim($arr[0]) == "TableNum"){
			$value = substr(trim($arr[1]), 1, strlen(trim($arr[1]))-2);
			$value2 = explode(',', $value);
			$v1 = trim($value2[0]);
			$v2 = trim($value2[1]);
			$tDtl['TABLE_DIV_NUM'] = $v1;
			$tDtl['TABLE_MOD_NUM'] = $v2;
		}
		else if(trim($arr[0]) == "FieldName"){
			$value = trim($arr[1]);
			if(!$flag){
				$tDtl['FIELDS'] = array();
				$tDtl['TTCKEY'] = $value;
				$flag = true;
			}
			$tDtl['FIELDS'][$last_index] = array();
			$tDtl['FIELDS'][$last_index]['name'] = "$value";
			$tDtl['FIELDS'][$last_index]['cname'] = "$value";
			$tDtl['FIELDS'][$last_index]['auto'] = 0;
			$tDtl['FIELDS'][$last_index]['primary'] = 0;
		}
		else if(trim($arr[0]) == "FieldType"){
			$value = trim($arr[1]);
			$tDtl['FIELDS'][$last_index]['type'] = "$value";
		}
		else if(trim($arr[0]) == "FieldSize"){
			$value = trim($arr[1]);
			$tDtl['FIELDS'][$last_index]['size'] = "$value";
			++$last_index;
		}
	}
}