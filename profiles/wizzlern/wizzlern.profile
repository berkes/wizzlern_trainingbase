<?php
// $Id: default.profile,v 1.22 2007/12/17 12:43:34 goba Exp $

/**
 * Return an array of the modules to be enabled when this profile is installed.
 *
 * @return
 *   An array of modules to enable.
 */
function wizzlern_profile_modules() {
  //Required modules
  return array(
    //Required core modules
    1 => 'color',
    2 => 'comment',
    3 => 'help',
    4 => 'menu',
    5 => 'taxonomy',
    6 => 'dblog',

    //Optional modules
    100 => 'book',
    101 => 'forum',
    102 => 'upload',
    103 => 'statistics',
    104 => 'search',
    105 => 'path',
    106 => 'openid',
    107 => 'trigger',
    108 => 'contact',

    //Enable Contributed modules
    1001 => 'token',
    1002 => 'pathauto',
    1003 => 'tagadelic',
    1004 => 'imageapi',
    1005 => 'imagecache',
    1006 => 'imagecache_ui',
    1007 => 'scheduler',
    1008 => 'admin_menu',

    1010 => 'views',//Views
    1011 => 'views_ui',

    1020 => 'content',//CCK
    1021 => 'content_copy',
    1022 => 'content_permissions',
    1023 => 'fieldgroup',
    1024 => 'nodereference',
    1025 => 'number',
    1026 => 'optionwidgets',
    1027 => 'text',
    1028 => 'userreference',
    1029 => 'filefield',
    1030 => 'imagefield',
    1031 => 'date_api',
    1032 => 'date_timezone',
    1033 => 'date',
    1034 => 'date_tools',

    //Enable Development modules
    //2001 => 'devel',
    //2002 => 'devel_themer',
    //2003 => 'macro',
    //2010 => 'drush',
    //2011 => 'drush_pm',
    //2012 => 'drush_pm_wget',
    //2013 => 'drush_pm_svn',
    //2014 => 'drush_sql',
    //2015 => 'drush_tools',
  );
}

/**
 * Return a description of the profile for the initial installation screen.
 *
 * @return
 *   An array with keys 'name' and 'description' describing this profile,
 *   and optional 'language' to override the language selection for
 *   language-specific profiles.
 */
function wizzlern_profile_details() {
  return array(
    'name' => 'Drupal for Wizzlern Training',
    'description' => 'Select this profile as base for your Wizzlern training.'
  );
}

/**
 * Return a list of tasks that this profile supports.
 *
 * @return
 *   A keyed array of tasks the profile will perform during
 *   the final stage. The keys of the array will be used internally,
 *   while the values will be displayed to the user in the installer
 *   task list.
 */
function wizzlern_profile_task_list() {
}

/**
 * Perform any final installation tasks for this profile.
 *
 * The installer goes through the profile-select -> locale-select
 * -> requirements -> database -> profile-install-batch
 * -> locale-initial-batch -> configure -> locale-remaining-batch
 * -> finished -> done tasks, in this order, if you don't implement
 * this function in your profile.
 *
 * If this function is implemented, you can have any number of
 * custom tasks to perform after 'configure', implementing a state
 * machine here to walk the user through those tasks. First time,
 * this function gets called with $task set to 'profile', and you
 * can advance to further tasks by setting $task to your tasks'
 * identifiers, used as array keys in the hook_profile_task_list()
 * above. You must avoid the reserved tasks listed in
 * install_reserved_tasks(). If you implement your custom tasks,
 * this function will get called in every HTTP request (for form
 * processing, printing your information screens and so on) until
 * you advance to the 'profile-finished' task, with which you
 * hand control back to the installer. Each custom page you
 * return needs to provide a way to continue, such as a form
 * submission or a link. You should also set custom page titles.
 *
 * You should define the list of custom tasks you implement by
 * returning an array of them in hook_profile_task_list(), as these
 * show up in the list of tasks on the installer user interface.
 *
 * Remember that the user will be able to reload the pages multiple
 * times, so you might want to use variable_set() and variable_get()
 * to remember your data and control further processing, if $task
 * is insufficient. Should a profile want to display a form here,
 * it can; the form should set '#redirect' to FALSE, and rely on
 * an action in the submit handler, such as variable_set(), to
 * detect submission and proceed to further tasks. See the configuration
 * form handling code in install_tasks() for an example.
 *
 * Important: Any temporary variables should be removed using
 * variable_del() before advancing to the 'profile-finished' phase.
 *
 * @param $task
 *   The current $task of the install system. When hook_profile_tasks()
 *   is first called, this is 'profile'.
 * @param $url
 *   Complete URL to be used for a link or form action on a custom page,
 *   if providing any, to allow the user to proceed with the installation.
 *
 * @return
 *   An optional HTML string to display to the user. Only used if you
 *   modify the $task, otherwise discarded.
 */
function wizzlern_profile_tasks(&$task, $url) {

  // Insert default user-defined node types into the database. For a complete
  // list of available node type attributes, refer to the node type API
  // documentation at: http://api.drupal.org/api/HEAD/function/hook_node_info.
  $types = array(
    array(
      'type' => 'page',
      'name' => st('Page'),
      'module' => 'node',
      'description' => st("A <em>page</em>, similar in form to a <em>story</em>, is a simple method for creating and displaying information that rarely changes, such as an \"About us\" section of a website. By default, a <em>page</em> entry does not allow visitor comments and is not featured on the site's initial home page."),
      'custom' => TRUE,
      'modified' => TRUE,
      'locked' => FALSE,
      'help' => '',
      'min_word_count' => '',
    ),
    array(
      'type' => 'event',
      'name' => st('Event'),
      'module' => 'node',
      'description' => st("An <em>event</em>, is ideal for creating and displaying content with a date in agenda's, calendars and event-lists. By default, an <em>event</em> is automatically featured on the site's initial home page, and provides the ability to post comments."),
      'custom' => TRUE,
      'modified' => TRUE,
      'locked' => FALSE,
      'help' => '',
      'min_word_count' => '',
    ),
  );

  foreach ($types as $type) {
    $type = (object) _node_type_set_defaults($type);
    node_type_save($type);
  }

  // Default page to not be promoted and have comments disabled.
  variable_set('node_options_page', array('status'));
  variable_set('comment_page', COMMENT_NODE_DISABLED);

  // Don't display date and author information for page nodes by default.
  $theme_settings = variable_get('theme_settings', array());
  $theme_settings['toggle_node_info_page'] = FALSE;
  variable_set('theme_settings', $theme_settings);

  // Update the menu router information.
  menu_rebuild();
}

/**
 * Implementation of hook_form_alter().
 *
 * Allows the profile to alter the site-configuration form. This is
 * called through custom invocation, so $form_state is not populated.
 */
function wizzlern_form_alter(&$form, $form_state, $form_id) {
  if ($form_id == 'install_configure') {
    // Set default for site name field.
    $form['site_information']['site_name']['#value'] = $_SERVER['SERVER_NAME'] . '/Training';
  }
}

