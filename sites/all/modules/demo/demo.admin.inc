<?php
// $Id: demo.admin.inc,v 1.1.2.7 2009/02/06 21:04:08 smk Exp $

/**
 * @file
 * Demonstration Site administrative pages
 */

/**
 * Current version of SQL dump structure.
 */
define('DEMO_DUMP_VERSION', '1.1');

function demo_admin_settings() {
  global $base_url;
  
  $form['status'] = array(
    '#type' => 'fieldset',
    '#title' => t('Status'),
    '#collapsible' => false,
  );
  if (variable_get('demo_reset_last', 0)) {
    $reset_date = format_date(variable_get('demo_reset_last', 0));
  }
  else {
    $reset_date = t('Never');
  }
  $form['status'][] = array(
    '#value' => t('<p><strong>Last reset:</strong> !date</p>', array('!date' => $reset_date)),
  );
  $form['status'][] = array(
    '#value' => t('<p><strong>Default snapshot:</strong> !snapshot</p>', array('!snapshot' => variable_get('demo_dump_cron', t('None')))),
  );
  
  $fileconfig = demo_get_fileconfig();
  
  $form['dump'] = array(
    '#type' => 'fieldset',
    '#title' => t('Dump settings'),
    '#collapsible' => true,
    '#collapsed' => (variable_get('demo_reset_interval', 0) ? false : true),
  );
  $period = drupal_map_assoc(array(0, 1800, 3600, 7200, 10800, 14400, 18000, 21600, 32400, 43200, 86400, 172800, 259200, 604800, 1209600, 2419200, 4838400, 9676800), 'format_interval');
  $period[0] = t('disabled');
  $form['dump']['interval'] = array(
    '#type' => 'select',
    '#title' => t('Automatically reset site every'),
    '#default_value' => variable_get('demo_reset_interval', 0),
    '#options' => $period,
    '#description' => t('Select how often this demonstration site is automatically reset. Ensure that you have chosen a snapshot for cron runs in <a href="!manage">Manage snapshots</a> first. <strong>Note:</strong> This requires cron to run at least within this interval.', array('!manage' => url('admin/build/demo/manage'))),
  );
  
  $form['dump']['path'] = array(
    '#type' => 'textfield',
    '#title' => t('Dump path'),
    '#default_value' => $fileconfig['path'],
    '#size' => 30,
    '#description' => t('Enter a writable directory where dump files of this demonstration site are stored, f.e. %files. The name of this site (e.g. %confpath) is automatically appended to this directory.<br /><br /><strong>Note: For security reasons you should store site dumps outside of the document root of your webspace!</strong>', array('%files' => file_directory_path() .'/demo', '%confpath' => $fileconfig['site'])),
  );
  $form[] = array(
    '#type' => 'submit',
    '#value' => t('Save'),
  );
  
  return $form;
}

function demo_admin_settings_submit($form, &$form_state) {
  if (!file_check_directory($form_state['values']['path'], FILE_CREATE_DIRECTORY)) {
    form_set_error('path', t('The snapshot directory %directory could not be created.', array('%directory' => $form_state['values']['path'])));
    return FALSE;
  }
  else {
    variable_set('demo_dump_path', $form_state['values']['path']);
  }
  variable_set('demo_reset_interval', $form_state['values']['interval']);
  drupal_set_message(t('The configuration options have been saved.'));
}

function demo_manage() {
  $form['dump'] = array(
    '#type' => 'fieldset',
    '#title' => t('Available snapshots'),
  );
  $form = array_merge_recursive($form, demo_get_dumps());
  $form[] = array(
    '#type' => 'submit',
    '#value' => t('Set as default snapshot for cron'),
  );
  $form[] = array(
    '#type' => 'submit',
    '#value' => t('Delete selected snapshot'),
    '#submit' => array('demo_manage_delete_submit'),
  );
  return $form;
}

function demo_manage_submit($form, &$form_state) {
  demo_set_default($form_state['values']['filename']);
}

function demo_manage_delete_submit($form, &$form_state) {
  $form_state['redirect'] = 'admin/build/demo/delete/'. $form_state['values']['filename'];
}

function demo_delete_confirm(&$form_state, $filename) {
  $fileconfig = demo_get_fileconfig($filename);
  if (!file_exists($fileconfig['infofile'])) {
    return drupal_access_denied();
  }

  $form['filename'] = array(
    '#type' => 'value',
    '#value' => $filename,
  );
  return confirm_form($form, t('Are you sure you want to delete the snapshot %title?', array('%title' => $filename)), 'admin/build/demo/manage', t('This action cannot be undone.'), t('Delete'));
}

function demo_delete_confirm_submit($form, &$form_state) {
  $files = demo_get_fileconfig($form_state['values']['filename']);
  unlink($files['sqlfile']);
  unlink($files['infofile']);
  drupal_set_message(t('Snapshot %title has been deleted.', array('%title' => $form_state['values']['filename'])));
  $form_state['redirect'] = 'admin/build/demo/manage';
}

function demo_dump() {
  $form = array();
  $form['dump']['filename'] = array(
    '#title' => t('File name'),
    '#type' => 'textfield',
    '#autocomplete_path' => 'demo/autocomplete',
    '#required' => true,
    '#maxlength' => 128,
    '#description' => t('Enter the snapshot file name without file extension. Allowed characters are a-z, 0-9, dashes ("-"), underscores ("_") and dots.'),
  );
  $form['dump']['default'] = array(
    '#title' => t('Set as new default snapshot'),
    '#type' => 'checkbox',
  );
  $form['dump']['description'] = array(
    '#title' => t('Description'),
    '#type' => 'textarea',
    '#rows' => 2,
    '#description' => t('Optionally enter a description for this snapshot here. If no description is given and a snapshot with the same filename already exists, the previous description is used.'),
  );
  return confirm_form($form, t('Are you sure you want to create a new snapshot?'), 'admin/build/demo', t('If the above filename already exists, creating a new snapshot will overwrite the existing snapshot. This action cannot be undone.'), t('Create'), t('Cancel'));
}

function demo_dump_submit($form, &$form_state) {
  global $db_type;

  // Generate info file.
  $info = demo_set_info($form_state['values']);
  if (!$info) {
    return false;
  }
  
  // Include database specific functions.
  $engine = ($db_type == 'mysqli' ? 'mysql' : $db_type);
  $inc_file = drupal_get_path('module', 'demo') .'/database_'. $engine .'_dump.inc';
  if (file_exists($inc_file)) {
    require_once $inc_file;

    if (!empty($form_state['values']['default'])) {
      // Set new default snapshot.
      demo_set_default($info['filename']);
    }
  
    // Increase PHP's max_execution_time for large dumps.
    @set_time_limit(600);
  
    // Perform dump.
    $fileconfig = demo_get_fileconfig($info['filename']);
    $exclude = array('{cache}', '{cache_content}', '{cache_filter}', '{cache_menu}', '{cache_page}', '{cache_views}', '{panels_object_cache}', '{watchdog}');
    $exclude = array_map('db_prefix_tables', $exclude);
    demo_dump_db($fileconfig['sqlfile'], $exclude);
    drupal_set_message(t('Successfully created snapshot %filename.', array('%filename' => $fileconfig['sqlfile'])));
  }
  else {
    drupal_set_message(t('@engine support not implemented yet.', array('@engine' => ucfirst($engine))), 'error');
  }

  $form_state['redirect'] = 'admin/build/demo/manage';
}

function demo_reset_confirm() {
  $form['dump'] = array(
    '#type' => 'fieldset',
    '#title' => t('Available snapshots'),
  );
  $form = array_merge_recursive($form, demo_get_dumps());
  
  return confirm_form($form, t('Are you sure you want to reset the site?'), 'admin/build/demo', t('Resetting the site will overwrite all changes that have been made to this Drupal installation since the chosen snapshot.<br /><br /><div style="color: red; font-weight: bold; font-size: 18px;"><center>THIS ACTION CANNOT BE UNDONE!</center><br /></div>'), t('Reset'), t('Cancel'));
}

function demo_reset_confirm_submit($form, &$form_state) {
  // Increase PHP's max_execution_time for large dumps.
  @set_time_limit(600);

  // Reset site to chosen snapshot.
  demo_reset($form_state['values']['filename']);
  // Save time of last reset.
  variable_set('demo_reset_last', time());
  
  $form_state['redirect'] = isset($form_state['values']['redirect']) ? $form_state['values']['redirect'] : 'admin/build/demo';
}

function demo_reset($filename = 'demo_site', $verbose = TRUE) {
  $fileconfig = demo_get_fileconfig($filename);
  if (!file_exists($fileconfig['sqlfile']) || !($fp = fopen($fileconfig['sqlfile'], 'r'))) {
    if ($verbose) {
      drupal_set_message(t('Unable to open dump file %filename.', array('%filename' => $fileconfig['sqlfile'])), 'error');
    }
    watchdog('demo', 'Unable to open dump file %filename.', array('%filename' => $fileconfig['sqlfile']), WATCHDOG_ERROR);
    return FALSE;
  }

  // Load any database information in front of reset.
  $demo_dump_cron = variable_get('demo_dump_cron', $filename);
  $version = demo_get_info($fileconfig['infofile'], 'version');
  $is_version_1_0_dump = version_compare($version, '1.1', '<');

  // Drop tables
  $dt_watchdog = db_prefix_tables('{watchdog}');
  foreach (demo_enum_tables() as $table) {
    // Skip watchdog, except for legacy dumps that included the watchdog table
    if ($table != $dt_watchdog || $is_version_1_0_dump) {
      db_query("DROP TABLE %s", $table);
    }
  }

  // Load data from snapshot.
  $success = TRUE;
  $query = '';
  $new_line = TRUE;

  while (!feof($fp)) {
    // Better performance on PHP 5.2.x when leaving out buffer size to
    // fgets().
    $data = fgets($fp);
    if ($data === FALSE) {
      break;
    }
    // Skip empty lines (including lines that start with a comment).
    if ($new_line && ($data == "\n" || !strncmp($data, '--', 2) || !strncmp($data, '#', 1))) {
      continue;
    }

    $query .= $data;
    $len = strlen($data);
    if ($data[$len - 1] == "\n") {
      if ($data[$len - 2] == ';') {
        // Reached the end of a query, now execute it.
        if (!_db_query($query, FALSE)) {
          $success = FALSE;
        }
        $query = '';
      }
      $new_line = TRUE;
    }
    else {
      // Continue adding data from the same line.
      $new_line = FALSE;
    }
  }
  fclose($fp);

  if ($success) {
    if ($verbose) {
      drupal_set_message(t('Successfully restored database from %filename.', array('%filename' => $fileconfig['sqlfile'])));
    }
    watchdog('demo', 'Successfully restored database from %filename.', array('%filename' => $fileconfig['sqlfile']), WATCHDOG_NOTICE);
  }
  else {
    if ($verbose) {
      drupal_set_message(t('Failed restoring database from %filename.', array('%filename' => $fileconfig['sqlfile'])), 'error');
    }
    watchdog('demo', 'Failed restoring database from %filename.', array('%filename' => $fileconfig['sqlfile']), WATCHDOG_ERROR);
  }

  // Reset default dump to load on cron.
  variable_set('demo_dump_cron', $demo_dump_cron);

  return $success;
}

function demo_get_fileconfig($filename = 'demo_site') {
  $fileconfig = array();
  
  // Build dump path.
  $fileconfig['path'] = variable_get('demo_dump_path', file_directory_path() .'/demo');
  $fileconfig['site'] = str_replace('sites', '', conf_path());
  $fileconfig['dumppath'] = $fileconfig['path'] . $fileconfig['site'];
  
  // Check if directory exists.
  file_check_directory($fileconfig['path'], FILE_CREATE_DIRECTORY, 'path');
  if (!file_check_directory($fileconfig['dumppath'], FILE_CREATE_DIRECTORY, 'path')) {
    return false;
  }
  
  // Protect dump files.
  $htaccess = $fileconfig['path'] ."/.htaccess";
  if (!is_file($htaccess)) {
    $htaccess_lines = "# demo.module snapshots\n# Do not let the webserver serve anything under here!\n#\nDeny from all\n";
    if (($fp = fopen($htaccess, 'w')) && fputs($fp, $htaccess_lines)) {
      fclose($fp);
      chmod($htaccess, 0664);
    }
  }
    
  // Build SQL filename.
  $fileconfig['sql'] = $filename .'.sql';
  $fileconfig['sqlfile'] = $fileconfig['path'] . $fileconfig['site'] .'/'. $filename .'.sql';
  
  // Build info filename.
  $fileconfig['info'] = $filename .'.info';
  $fileconfig['infofile'] = $fileconfig['path'] . $fileconfig['site'] .'/'. $filename .'.info';
  
  return $fileconfig;
}

function demo_get_dumps() {
  $fileconfig = demo_get_fileconfig();
  
  // Fetch list of available info files
  $files = file_scan_directory($fileconfig['dumppath'], '.info$');

  foreach ($files as $file => $object) {
    $files[$file]->filemtime = filemtime($file);
    $files[$file]->filesize = filesize(substr($file, 0, -4) .'sql');
  }

  // Sort snapshots by date (ascending file modification time)
  uasort($files, create_function('$a, $b', 'return ($a->filemtime < $b->filemtime);'));
  
  $options = array();
  // Forms API does not pass selected value of individual radio buttons,
  // so we manually insert an internal form value here.
  $options['dump']['filename'] = array(
    '#type' => 'value',
    '#required' => true,
    '#title' => t('Snapshot'),
  );
  foreach ($files as $filename => $file) {
    // Build basic file info
    $files[$filename] = (array)$files[$filename];
    $info = demo_get_info($filename);
    
    // Convert file info for Forms API
    $option = array();
    $option['#type'] = 'radio';
    $option['#name'] = 'filename';
    $option['#title'] = $info['filename'] .' ('. format_date($file->filemtime, 'small') .', '. format_size($file->filesize) .')';
    $option['#return_value'] = $info['filename'];
    if ($info['filename'] == variable_get('demo_dump_cron', 'demo_site')) {
      $option['#value'] = $info['filename'];
    }
    $option['#description'] = '';
    if (!empty($info['description'])) {
      $option['#description'] .= $info['description'] .'<br /><br />';
    }
    if (count($info['modules']) > 1) {
      // Remove required core modules and obvious modules from module list.
      $info['modules'] = array_diff($info['modules'], array('block', 'filter', 'node', 'system', 'user', 'watchdog', 'demo'));
      // Sort module list alphabetically.
      sort($info['modules']);
      $option['#description'] .= t('Modules: ') . implode(', ', $info['modules']);
    }
    $option['#attributes'] = array('onclick' => "$('.description', this.parentNode.parentNode).slideToggle();");
    
    $options['dump'][] = $option;
  }
  
  // Attach stylesheet to initially hide descriptions
  drupal_add_js("$('div.form-item div.description', $('form')).hide();", 'inline', 'footer');
  
  return $options;
}

function demo_get_info($filename, $field = NULL) {
  $info = array();

  if (file_exists($filename)) {
    $info = parse_ini_file($filename);

    if (isset($info['modules'])) {
      $info['modules'] = explode(" ", $info['modules']);
    }
    else {
      $info['modules'] = null;
    }

    if (!isset($info['version'])) {
      $info['version'] = '1.0';
    }
  }

  if (isset($field)) {
    return isset($info[$field]) ? $info[$field] : NULL;
  }
  else {
    return $info;
  }
}

function demo_set_info($values = null) {
  if (isset($values['filename']) && is_array($values)) {
    // Check for valid filename
    if (!preg_match('/^[-_\.a-zA-Z0-9]+$/', $values['filename'])) {
      drupal_set_message(t('Dump filename %title must contain alphanumeric characters, dots, dashes and underscores only. Other characters, including blanks (spaces), are not allowed.', array('%title' => $values['filename'])), 'error');
      return false;
    }

    if (!empty($values['description'])) {
      // parse_ini_file() doesn't allow certain characters in description
      $s = array("\r\n", "\r", "\n", '"');
      $r = array(' ', ' ', ' ', "'");
      $values['description'] = str_replace($s, $r, $values['description']);
    }
    else {
      // If new description is empty, try to use previous description.
      $old_file = demo_get_fileconfig($values['filename']);
      $old_description = demo_get_info($old_file['infofile'], 'description');
      if (!empty($old_description)) {
        $values['description'] = $old_description;
      }
    }
    
    // Set values
    $infos = array();
    $infos['filename'] = $values['filename'];
    $infos['description'] = '"'. $values['description'] .'"';
    $infos['modules'] = implode(' ', module_list());
    $infos['version'] = DEMO_DUMP_VERSION;

    // Write information to .info file
    $fileconfig = demo_get_fileconfig($values['filename']);
    $infofile = fopen($fileconfig['infofile'], 'w');
    foreach ($infos as $key => $info) {
      fwrite($infofile, $key .' = '. $info ."\n");
    }
    fclose($infofile);
    
    return $infos;
  }
}

/**
 * Returns a list of tables in the active database.
 *
 * Only returns tables whose prefix matches the configured one (or ones, if
 * there are multiple).
 */
function demo_enum_tables() {
  global $db_prefix;

  $tables = array();

  if (is_array($db_prefix)) {
    // Create a regular expression for table prefix matching.
    $rx = '/^'. implode('|', array_filter($db_prefix)) .'/';
  }
  else if ($db_prefix != '') {
    $rx = '/^'. $db_prefix .'/';
  }

  switch ($GLOBALS['db_type']) {
    case 'mysql':
    case 'mysqli':
      $result = db_query("SHOW TABLES");
      break;
    case 'pgsql':
      $result = db_query("SELECT table_name FROM information_schema.tables WHERE table_schema = '%s'", 'public');
      break;
  }

  while ($table = db_fetch_array($result)) {
    $table = reset($table);
    if (is_array($db_prefix)) {
      // Check if table name matches a configured prefix.
      if (preg_match($rx, $table, $matches)) {
        $table_prefix = $matches[0];
        $plain_table = substr($table, strlen($table_prefix));
        if ($db_prefix[$plain_table] == $table_prefix || $db_prefix['default'] == $table_prefix) {
          $tables[] = $table;
        }
      }
    }
    else if ($db_prefix != '') {
      if (preg_match($rx, $table)) {
        $tables[] = $table;
      }
    }
    else {
      $tables[] = $table;
    }
  }

  return $tables;
}

/**
 * Retrieve a pipe delimited string of autocomplete suggestions for existing
 * snapshot names.
 */
function demo_autocomplete($string = '') {
  $matches = array();
  if ($string && $fileconfig = demo_get_fileconfig()) {
    $string = preg_quote($string);
    $files = file_scan_directory($fileconfig['dumppath'], $string .'.*\.info$');
    foreach ($files as $file) {
      $matches[$file->name] = check_plain($file->name);
   }
  }
  print drupal_to_js($matches);
  exit();
}

/**
 * Sets a specific snapshot as default for cron runs or the site reset block.
 *
 * @param $filename
 *   The filename of the snapshot.
 */
function demo_set_default($filename) {
  variable_set('demo_dump_cron', $filename);
  drupal_set_message(t('Snapshot %title will be used for upcoming cron runs.', array('%title' => $filename)));
}

