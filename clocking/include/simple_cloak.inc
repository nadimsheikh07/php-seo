<?php

/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

// load configuration file
require_once('config.inc');

class SimpleCloak {

// returns the confidence level
    function isSpider($spider_name = '', $check_uas = true, $check_ips = true) {
// default confidence level to 0
        $confidence = 0;
// matching user agent?    
        if ($check_uas)
            if (SimpleCloak::_get(0, $spider_name, 'UA', $_SERVER['HTTP_USER_AGENT']))
                $confidence += 2;
// matching IP?    
        if ($check_ips)
            if (SimpleCloak::_get(0, $spider_name, 'IP', '', $_SERVER['REMOTE_ADDR']))
                $confidence += 3;
// return confidence level
        return $confidence;
    }

// retrieve cloaking data filtered by the supplied parameters
    function _get($id = 0, $spider_name = '', $record_type = '', $value = '', $wildcard_value = '') {
        // Connect to MySQL server
        $dbLink = mysqli_connect(DB_HOST, DB_USER, DB_PASSWORD)
                or die("Could not connect: " . mysqli_error());

// by default, retrieve all records
        $q = " SELECT cloak_data.* FROM cloak_data WHERE TRUE ";
// add filters
        if ($id) {
            $id = (int) $id;
            $q .= " AND id = $id ";
        }
        if ($spider_name) {
            $spider_name = mysqli_escape_string($dbLink, $spider_name);
            $q .= " AND spider_name = '$spider_name' ";
        }
        if ($record_type) {
            $record_type = mysqli_escape_string($dbLink, $record_type);
            $q .= " AND record_type = '$record_type' ";
        }
        if ($value) {
            $value = mysqli_escape_string($dbLink, $value);
            $q .= " AND value = '$value' ";
        }

        if ($wildcard_value) {
            $wildcard_value = mysqli_escape_string($dbLink, $wildcard_value);
            $q .= " AND '$wildcard_value' LIKE CONCAT(value, '%') ";
        }

// Connect to the seophp database 
        mysqli_select_db($dbLink, DB_DATABASE) or die("Could not select database");
// execute the query
        $tmp = mysqli_query($dbLink, $q);
// close database connection 
        mysqli_close($dbLink);
// return the results as an associative array
        $rows = array();
        while ($_x = mysqli_fetch_assoc($tmp)) {
            $rows[] = $_x;
        }
        return $rows;
    }

// updates the entire database with fresh spider data, but only if our data is
// more than 7 days old, and if the online version from iplists.org has changed
    function updateAll() {
// Connect to MySQL server

        $dbLink = mysqli_connect(DB_HOST, DB_USER, DB_PASSWORD)
                or die("Could not connect: " . mysqli_error());
// Connect to the seophp database 
        mysqli_select_db($dbLink, DB_DATABASE) or die("Could not select database");
// retrieve last update information from database
        $q = "SELECT cloak_update.* FROM cloak_update";
        $tmp = mysqli_query($dbLink, $q);
        $updated = mysqli_fetch_assoc($tmp);
        $db_version = $updated['version'];
        $updated_on = $updated ['updated_on'];
// get the latest update more recent than 7 days, don't attempt an update    
        if (isset($updated_on) &&
                (strtotime($updated_on) > strtotime("-604800 seconds"))) {
// close database connection 
            mysqli_close($dbLink);
// return false to indicate an update wasn't performed
            return false;
        }
        // read the latest iplists version     
        $version_url = 'http://www.iplists.com/nw/version.php';

        $latest_version = mysqli_escape_string($dbLink, file_get_contents($version_url));
// if no updated version information was retrieved, abort
        if (!$latest_version) {
// return false to indicate an update wasn't performed
            return false;
        }
// save the update data
        $q = "DELETE FROM cloak_update";
        mysqli_query($dbLink, $q);
        $q = "INSERT INTO cloak_update (version, updated_on) VALUES('$latest_version', NOW())";
        mysqli_query($dbLink, $q);
// if we already have the current data, don't attempt an update 
        if ($latest_version == $db_version) {
// close database connection 
            mysqli_close($dbLink);
// return false to indicate an update wasn't performed
            return false;
        }
// update the database
        SimpleCloak::_updateCloakingDB('google', 'http://www.iplists.com/nw/google.txt');
        SimpleCloak::_updateCloakingDB('yahoo', 'http://www.iplists.com/nw/inktomi.txt');
        SimpleCloak::_updateCloakingDB('msn', 'http://www.iplists.com/nw/msn.txt');
        SimpleCloak::_updateCloakingDB('ask', 'http://www.iplists.com/nw/askjeeves.txt');
        SimpleCloak::_updateCloakingDB('altavista', 'http://www.iplists.com/nw/altavista.txt');
        SimpleCloak::_updateCloakingDB('lycos', 'http://www.iplists.com/nw/lycos.txt');
        SimpleCloak::_updateCloakingDB('wisenut', 'http://www.iplists.com/nw/wisenut.txt');
// close connection 
        mysqli_close($dbLink);
// return true to indicate a successful update
        return true;
    }

// update the database for the mentioned spider, by reading the provided URL
    function _updateCloakingDB($spider_name, $url, $ua_regex = '/^# UA "(.*)"$/m', $ip_regex = '/^([0-9.]+)$/m') {
        $dbLink = mysqli_connect(DB_HOST, DB_USER, DB_PASSWORD)
                or die("Could not connect: " . mysqli_error());
// use cURL to read the data from $url
// NOTE: additional settings are required when accessing the web through a proxy
        $ch = curl_init();
        curl_setopt($ch, CURLOPT_URL, $url);
        curl_setopt($ch, CURLOPT_HEADER, 1);
        curl_setopt($ch, CURLOPT_RETURNTRANSFER, 1);
        curl_setopt($ch, CURLOPT_FOLLOWLOCATION, 1);
        curl_setopt($ch, CURLOPT_TIMEOUT, 60);
        $result = curl_exec($ch);
        curl_close($ch);
// use _parseListURL to parse the list of IPs and user agents
        $lists = SimpleCloak::_parseListURL($result, $ua_regex, $ip_regex);
        
       
// if the user agents and IPs weren't retrieved, we cancel the update 
        if (!$lists['ua_list'] || !$lists['ip_list'])
            return;
// lock the cloack_data table to avoid concurrency problems
        mysqli_query($dbLink, 'LOCK TABLES cloak_data WRITE');
// delete all the existing data for $spider_name
        SimpleCloak::_deleteSpiderData($spider_name);
// insert the list of user agents for the spider
        foreach ($lists['ua_list'] as $ua) {
             
            SimpleCloak::_insertSpiderData($spider_name, 'UA', $ua);
        }
// insert the list of IPs for the spider
        foreach ($lists['ip_list'] as $ip) {
            SimpleCloak::_insertSpiderData($spider_name, 'IP', $ip);
        }
// release the table lock
        mysqli_query($dbLink, 'UNLOCK TABLES');
    }

// helper function used to parse lists of user agents and IPs
    function _parseListURL($data, $ua_regex, $ip_regex) {
        $ua_list_ret = preg_match_all($ua_regex, $data, $ua_list);
        $ip_list_ret = preg_match_all($ip_regex, $data, $ip_list);
        return array('ua_list' => $ua_list[1], 'ip_list' => $ip_list[1]);
    }

// inserts a new row of data to the cloaking table
    function _insertSpiderData($spider_name, $record_type, $value) {
        $dbLink = mysqli_connect(DB_HOST, DB_USER, DB_PASSWORD)
                or die("Could not connect: " . mysqli_error());
// Connect to the seophp database 
        mysqli_select_db($dbLink, DB_DATABASE) or die("Could not select database");
// escape input data
        $spider_name = mysqli_escape_string($dbLink, $spider_name);
        $record_type = mysqli_escape_string($dbLink, $record_type);
        $value = mysqli_escape_string($dbLink, $value);

// build and execute the INSERT query
        $q = "INSERT INTO cloak_data(spider_name,record_type,value) VALUES('$spider_name', '$record_type', '$value')";
        mysqli_query($dbLink, $q);
    }

// delete the cloaking data for the mentioned spider
    function _deleteSpiderData($spider_name) {
       $dbLink = mysqli_connect(DB_HOST, DB_USER, DB_PASSWORD)
                or die("Could not connect: " . mysqli_error());
// Connect to the seophp database 
        mysqli_select_db($dbLink, DB_DATABASE) or die("Could not select database");
// escape input data
        $spider_name = mysqli_escape_string($dbLink, $spider_name);
// build and execute the DELETE query
        $q = "DELETE FROM cloak_data WHERE spider_name = '$spider_name'";
        mysqli_query($dbLink, $q);
    }

}

?>
    