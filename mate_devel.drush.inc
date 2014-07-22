<?php

function mate_devel_drush_command()
{
  $items['mate-reset-pass'] = array(
    'callback' => 'mate_devel_reset_pass',
    'description' => 'Reset all the users\' passwords to 123 (by default), Also enabled any disabled account',
    'arguments' => array(
      'pass' => 'Optionally specify a non-default password.',
    ),
    'aliases' => array('nrp'),
  );

  $items['mate-get-files'] = array(
    'callback' => 'mate_devel_get_remote_files',
    'description' => 'Download files from other server',
    'arguments' => array(
      'server' => 'The production site url, by example http://server.nyx.com/',
    ),
    'options' => array(
      'proxy_url' => 'The proxy url, by example http://myproxy.corp.com:3128',
      'proxy_auth' => 'The credentials in the format [username]:[password]',
      'proxy_method' => 'NTLM or BASIC, NTLM by default',
      'images' => 'yes if you only need download only images, all for all files. By default all',
    ),
    'aliases' => array('grf'),
  );

  $items['mate-backpub-db'] = array(
    'callback' => 'mate_devel_backpub_db',
    'description' => 'Download DB from other server',
    'arguments' => array(
      'server' => 'The production site url, by example http://server.nyx.com/',
      'key' => 'The production site key to get access and download',
    ),
    'options' => array(
      'proxy_url' => 'The proxy url, by example http://myproxy.corp.com:3128',
      'proxy_auth' => 'The credentials in the format [username]:[password]',
      'proxy_method' => 'NTLM or BASIC, NTLM by default',
      'images' => 'yes if you only need download only images, all for all files. By default all',
    ),
    'aliases' => array('bckpub'),
  );

  $items['mate-delete-content'] = array(
    'callback' => 'mate_devel_delete_content',
    'description' => 'Deletes all nodes of the specified type, or all if no type is specified',
    'arguments' => array(
      'type' => 'Specify the node type to delete.',
    ),
    'aliases' => array('dc'),
  );

  $items['mate-delete-set-version'] = array(
    'callback' => 'mate_devel_set_version',
    'description' => 'Sets the schema version to a module',
    'arguments' => array(
      'module' => 'The module name',
      'version' => 'The version name',
    ),
    'aliases' => array('ssv'),
  );

  $items['mate-ga-info'] = array(
    'callback' => 'mate_gen_stats',
    'description' => 'Gets Info From GA',
    'arguments' => array(),
    'aliases' => array('mgai'),
  );

  return $items;
}

function mate_devel_delete_content($type = null)
{
  // Select the nodes that we want to delete.
  $result = db_select('node', 'n')
    ->fields('n', array('nid'));

  if (null !== $type) {
    $result->condition('type', $type, '=');
  }

  $deleted_count = 0;
  foreach ($result->execute() as $record) {
    node_delete($record->nid);
    $deleted_count++;
  }

  // Simple debug message so we can see what had been deleted.
  drush_print("$deleted_count nodes have been deleted");
}

function mate_devel_reset_pass($pass = 123)
{
  require_once 'includes/password.inc';
  $hpass = user_hash_password($pass);

  $ret = db_query("UPDATE {users} set pass = :pass, status = 1",
    array(':pass' => $hpass)
  );

  if ($ret) {
    drush_log(dt("Password changed and accounts activated"), 'success');
  } else {
    drush_log(dt("Password change unsuccessful"), 'error');
  }
}

function mate_devel_get_remote_files($server)
{
  require_once drupal_get_path('module', 'mate_devel') . '/Requester.php';

  $proxy_url = drush_get_option('proxy_url', null);
  $auth = drush_get_option('proxy_auth', null);
  $method = drush_get_option('proxy_method', 'NTLM');
  $only_images = drush_get_option('images', 'all');
  $url_parts = parse_url($server);
  $domain = $url_parts['host'];

  $requester = new Requester();
  if ($proxy_url !== null) {
    $requester->setOptionProxy(array(
      'url' => $proxy_url,
      'auth' => $auth,
      'auth_method' => $method
    ));
  }

  if (!$requester->ping($server)) {
    drush_log("Can't reach the server : $server", 'error');
    return;
  }

  $only_images_filter = '';
  if ($only_images === 'yes') {
    $only_images_filter = " AND filemime LIKE 'image/%' ";
  }

  $files = db_query("SELECT filename, uri FROM {file_managed}
      WHERE uri LIKE 'public://%' $only_images_filter");

  $files_path = conf_path() . '/files';

  if (!file_exists($files_path)) { //Creates files directory if wasn't created before.
    mkdir($files_path);
    chmod($files_path, 0777);
  }

  foreach ($files as $file) {
    $file_path = str_replace('public://', '', $file->uri);
    $file_path_enc = dirname($file_path);

    if ($file_path_enc == '.') {
      $file_path_enc = '';
    } else {
      $file_path_enc = '/' . $file_path_enc;
    }

    $file_path_enc .= '/' . rawurlencode($file->filename);

    $fp = $files_path . '/' . $file_path;
    $frp = "sites/$domain/files/" . $file_path_enc;

    if (!file_exists($fp)) { //If the file exists avoids the download
      if (!file_exists(dirname($fp))) {
        mkdir(dirname($fp), 0777, true);
      }

      $file_url = $server . '/' . $frp;

      try {
        $requester->setOptionResponseType(Requester::RESPONSE_RAW);
        $requester->save($fp, $file_url);
        chmod($fp, 0777);
        if (drush_get_context('DRUSH_VERBOSE')) {
          drush_log($fp);
        }
      } catch (Exception $e) {
        if (drush_get_context('DRUSH_VERBOSE')) {
          drush_log($e->getMessage(), 'error');
        }
      }
    }
  }

  if (drush_get_context('DRUSH_VERBOSE')) {
    drush_log('Done', 'success');
  }
}

function mate_devel_backpub_db($server, $key)
{
  require_once drupal_get_path('module', 'mate_devel') . '/Requester.php';

  global $base_url;
  $url_parts = parse_url($base_url);
  $domain = $url_parts['host'];

  $proxy_url = drush_get_option('proxy_url', null);
  $auth = drush_get_option('proxy_auth', null);
  $method = drush_get_option('proxy_method', 'NTLM');
  $local_db_file = '/tmp/db.sql';
  $db_url = $server . '/mate_devel/backup/' . $key;

  $command = _drush_sql_connect() . ' < ' . $local_db_file;

  $requester = new Requester();

  if ($proxy_url !== null) {
    $requester->setOptionProxy(array(
      'url' => $proxy_url,
      'auth' => $auth,
      'auth_method' => $method
    ));
  }

  if (!$requester->ping($server)) {
    drush_log("Can't reach the server : $server", 'error');
    return;
  }

  drush_log('Getting DB', 'success');
  $requester->setOptionResponseType(Requester::RESPONSE_RAW);
  $requester->save($local_db_file, $db_url);

  drush_log('Importing Db', 'success');
  shell_exec($command);

  drush_log('Reseting variables', 'success');
  //Reset some values for Dev
  variable_set('file_public_path', "sites/$domain/files");
  variable_set('file_private_path', "sites/$domain/private");
  variable_set('preprocess_css', 0);
  variable_set('preprocess_js', 0);
  variable_set('block_cache', 1);
  variable_set('cache', 0);

  drush_log('Reseting User passwords', 'success');
  mate_devel_reset_pass();

  drush_log('Getting files from server', 'success');
  mate_devel_get_remote_files($server); //Get Files

  drush_log('Done Db Import', 'success');
}


function mate_devel_set_version($module, $version)
{
  drupal_set_installed_schema_version($module, $version);
}