-- Author:   linjinming
-- Company:  JD China
-- Date:     2014-05-31
-- Database: dtc_agentm
-- Desc:     insert agent_conf sample

USE dtc_agentm;

insert into agent_conf set file='<? xml version="1.0" encoding="utf-8" ?>
<ALL>
    <AGENT_CONFIG ClientIdleTime="30" AdminListen="*:10101" Version="4" MasterAddr="127.0.0.1:11000"/>
    <BUSINESS_MODULE>
        <MODULE Id="101" ListenOn="127.0.0.1:10102">
            <CACHEINSTANCE Name="farm0" Addr="127.0.0.1:9898/tcp" />
            <!-- <CACHEINSTANCE Name="farm1" Addr="127.0.0.1:9899/tcp" /> -->
        </MODULE>
        <MODULE Id="102" ListenOn="127.0.0.1:10103">
            <CACHEINSTANCE Name="farm0" Addr="127.0.0.1:9898/tcp" />
            <!-- <CACHEINSTANCE Name="farm1" Addr="127.0.0.1:9899/tcp" /> -->
        </MODULE>

    </BUSINESS_MODULE>
</ALL>
',version=4,md5='123112312312312321313123123123123';