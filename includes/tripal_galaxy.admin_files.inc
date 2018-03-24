<?php
/**
 *
 * @return unknown
 */
function tripal_galaxy_admin_manage_files() {

  $headers = array('File', 'Short Desc', 'Path', 'Type', 'Enabled', 'Actions');
  $rows = array();

  $swfiles = db_select('tripal_galaxy_site_files', 'tgsf')
    ->fields('tgsf')
    ->orderBy('tgsf.filename')
    ->execute();

  while ($swfile = $swfiles->fetchObject()) {
    $filepath = '';
    if ($swfile->fid) {
      $file = file_load($swfile->fid);
      $filepath = $file->uri;
    }
    else {
      $filepath = $swfile->url;
    }
    $status_link = '';
    if ($swfile->enabled == 1) {
      $class = 'tripal-galaxy-enabled-file';
      $status_link = l('Disable', 'admin/tripal/extension/galaxy/files/disable/' . $swfile->swfid);
    }
    else {
      $class = 'tripal-galaxy-disabled-file';
      $status_link = l('Enable', 'admin/tripal/extension/galaxy/files/enable/' . $swfile->swfid);
    }
    $rows[] = array(
      'data' => array(
        $swfile->filename,
        $swfile->short_desc,
        $filepath,
        $swfile->filetype,
        $swfile->enabled ? 'Yes' : 'No',
        l('Edit', 'admin/tripal/extension/galaxy/files/edit/' . $swfile->swfid) . '&nbsp;|&nbsp;' .
        $status_link . '&nbsp;|&nbsp;' .
        l('Delete', 'admin/tripal/extension/galaxy/files/delete/' . $swfile->swfid),
      ),
      'class' => array($class),
    );
  }

  $content['instructions'] = array(
    '#type' => 'markup',
    '#markup' => '<p>' .  t('The following files will be made available to users inside of Galaxy workflows if the allowed types of the data input match the specified file type.') . '</p>',
  );
  $content['files_table'] = array(
    '#type' => 'item',
    '#title' => 'Files for Workflows',
    '#markup' => theme_table(array(
      'header' => $headers,
      'rows' => $rows,
      'attributes' => array(),
      'caption' => '',
      'colgroups' => array(),
      'sticky' => TRUE,
      'empty' => t('There are no site-wide files currently shared with for workflows.')
    )),
  );

  return $content;
}

/**
 * Implements the tripal_galaxy_admin_add_file_form form().
 */
function tripal_galaxy_admin_add_file_form($form, &$form_state) {
  $form['instructions'] = array(
    '#type' => 'markup',
    '#markup' => 'Use the following form to provide a new file to be made available to Galaxy workflows.  These files can be uploaded or provided via a remote URL.'
  );
  $form['file_upload']= array(
    '#type' => 'html5_file',
    '#title' => '',
    '#description' => 'Remember to click the "Upload" button below to send ' .
      'your file to the server.  This interface is capable of uploading very ' .
      'large files.  If you are disconnected you can return, reload the file and it ' .
      'will resume where it left off.  Once the file is uploaded the "Upload '.
      'Progress" will indicate "Complete".  If the file is already present on the server ' .
      'then the status will quickly update to "Complete".',
    '#usage_type' => 'tripal_galaxy',
    '#usage_id' => 0,
  );
//   $form['file_local']= array(
//     '#title' => t('Server path'),
//     '#type' => 'textfield',
//     '#maxlength' => 5120,
//     '#description'  => t('If the file is local to the Tripal server please provide the full path here.'),
//   );
  $form['file_remote']= array(
    '#title' => t('Remote path'),
    '#type' => 'textfield',
    '#maxlength' => 5102,
    '#description'  => t('If the file is available via a remote URL please provide the full URL here.  The file will be downloaded when the importer job is executed.'),
  );

  $form['file_type']= array(
    '#title' => t('Type'),
    '#type' => 'textfield',
    '#maxlength' => 32,
    '#size' => 15,
    '#required' => TRUE,
    '#description'  => t('The type of file (e.g. fasta, or fastq, etc.). This file can then be used for any Galaxy workflows that has this type in its list of allowed input types.'),
  );

  $form['short_desc']= array(
    '#title' => t('Short Description'),
    '#type' => 'textfield',
    '#maxlength' => 128,
    '#required' => TRUE,
    '#description'  => t('Please provide the description that will be shown to users for this file.'),
  );

  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Add File'),
    '#name' => 'submit-file',
  );

  $form['cancel'] = array(
    '#type' => 'markup',
    '#markup' => l('Cancel', 'admin/tripal/extension/galaxy/files'),
  );
  return $form;
}
/**
 * Implements the validate for the tripal_galaxy_admin_add_file_form form().
 */
function tripal_galaxy_admin_add_file_form_validate($form, &$form_state) {
  $short_desc = trim($form_state['values']['short_desc']);
  $file_type = trim($form_state['values']['file_type']);
  $file_remote = trim($form_state['values']['file_remote']);
  $file_upload = trim($form_state['values']['file_upload']);

  if (!$file_remote and !$file_upload) {
    form_set_error('file_remote', t('Please upload a file or provide the URL for a remote file.'));
  }
  if ($file_remote and $file_upload) {
    form_set_error('file_remote', t('Please provide only an uploaded file or a remote file, but not both.'));
  }

  // Make sure the URL is unique
  if ($file_remote) {
    $exists = db_select('tripal_galaxy_site_files', 'tgsf')
      ->fields('tgsf', array('swfid'))
      ->condition('url', $file_remote)
      ->execute()
      ->fetchField();
    if ($exists) {
      form_set_error('file_remote', t('This remote file has already been added.'));
    }
  }

  // Make sure the fid is unique
  if ($file_upload) {
    $exists = db_select('tripal_galaxy_site_files', 'tgsf')
      ->fields('tgsf', array('swfid'))
      ->condition('fid', $file_upload)
      ->execute()
      ->fetchField();
    if ($exists) {
      form_set_error('file_upload', t('The uploaded file has already been added.'));
    }
  }

}
/**
 * Implements the submit for the tripal_galaxy_admin_add_file_form form().
 */
function tripal_galaxy_admin_add_file_form_submit($form, &$form_state) {
  global $user;

  $short_desc = trim($form_state['values']['short_desc']);
  $file_type = strtolower(trim($form_state['values']['file_type']));
  $file_remote = trim($form_state['values']['file_remote']);
  $file_upload = trim($form_state['values']['file_upload']);

  $fields = array(
    'short_desc' => $short_desc,
    'filetype' => $file_type,
  );
  if ($file_remote) {
    $filename = basename($file_remote);
    $file = new stdClass();
    $file->uri = $file_remote;
    $file->filename = $filename;
    $file->filesize = 0;
    $file->filemime = file_get_mimetype($file_remote);
    $file->uid = $user->uid;
    $file->status = FILE_STATUS_PERMANENT;

    // The code below is copied from the file_save() function of Drupal
    // We don't call that function directly because it tries to use the
    // filesize() function which doesn't work for remote URLs.
    $file->timestamp = REQUEST_TIME;

    // Load the stored entity, if any.
    if (!empty($file->fid) && !isset($file->original)) {
      $file->original = entity_load_unchanged('file', $file->fid);
    }

    module_invoke_all('file_presave', $file);
    module_invoke_all('entity_presave', $file, 'file');

    if (empty($file->fid)) {
      drupal_write_record('file_managed', $file);
      // Inform modules about the newly added file.
      module_invoke_all('file_insert', $file);
      module_invoke_all('entity_insert', $file, 'file');
    }
    else {
      drupal_write_record('file_managed', $file, 'fid');
      // Inform modules that the file has been updated.
      module_invoke_all('file_update', $file);
      module_invoke_all('entity_update', $file, 'file');
    }

    // Clear internal properties.
    unset($file->original);
    // Clear the static loading cache.
    entity_get_controller('file')->resetCache(array($file->fid));

    $fields['url'] = $file_remote;
    $fields['filename'] = $filename;
    $fields['fid'] = $file->fid;
    file_usage_add($file, 'tripal_galaxy', 'site-wide', 0);
  }
  if ($file_upload) {
    $fields['fid'] = $file_upload;
    $file = file_load($file_upload);
    $filename = $file->filename;
    $fields['filename'] = $filename;
    // Move the file out of the user upload directory that the Tripal
    // upload tool uses and into a new directory that is site-specific
    // rather than user-specific.
    $site_dir = tripal_galaxy_get_files_dir();
    if (!$site_dir) {
      $message = 'Could not access the directory on the server for storing this file.';
      drupal_json_output(array(
        'status'  => 'failed',
        'message' => $message,
        'file_id' => '',
      ));
      return;
    }
    file_move($file, $site_dir . '/' . $filename);
  }

  // Add the file.
  $sfwid = db_insert('tripal_galaxy_site_files')
    ->fields($fields)
    ->execute();

  if ($sfwid) {
    drupal_set_message('Succesfully added new site-wide file for Galaxy workflows.');
    drupal_goto('admin/tripal/extension/galaxy/files');
  }
  else {
    drupal_set_message('A problem occured while trying to add the file.', 'error');
  }
}

/**
 * Implements the tripal_galaxy_admin_add_file_form form().
 */
function tripal_galaxy_admin_edit_file_form($form, &$form_state, $swfid) {

  $swfile = db_select('tripal_galaxy_site_files', 'tgsf')
     ->fields('tgsf')
     ->condition('swfid', $swfid)
     ->execute()
     ->fetchObject();

  $file_type = $swfile->filetype;
  $short_desc = $swfile->short_desc;

  if (array_key_exists('values', $form_state)) {
    $file_type = $form_state['values']['file_type'];
    $short_desc = $form_state['values']['short_desc'];
  }

  if ($swfile->fid) {
    $file = file_load($swfile->fid);
    $form['file_upload'] = array(
      '#type' => 'item',
      '#title' => 'Uloaded File',
      '#markup' => $file->filename,
    );
  }
  else {
    $form['file_remote'] = array(
      '#type' => 'item',
      '#title' => 'Remote File',
      '#markup' => $swfile->filename,
    );
  }

  $form['swfid'] = array(
    '#type' => 'value',
    '#value' => $swfid,
  );
  $form['file_type']= array(
    '#title' => t('Type'),
    '#type' => 'textfield',
    '#maxlength' => 32,
    '#size' => 15,
    '#required' => TRUE,
    '#default_value' => $file_type,
    '#description'  => t('The type of file (e.g. fasta, or fastq, etc.). This file can then be used for any Galaxy workflows that has this type in its list of allowed input types.'),
  );

  $form['short_desc']= array(
    '#title' => t('Short Description'),
    '#type' => 'textfield',
    '#maxlength' => 128,
    '#required' => TRUE,
    '#default_value' => $short_desc,
    '#description'  => t('Please provide the description that will be shown to users for this file.'),
  );

  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Update File'),
    '#name' => 'submit-file',
  );

  $form['cancel'] = array(
    '#type' => 'markup',
    '#markup' => l('Cancel', 'admin/tripal/extension/galaxy/files'),
  );
  return $form;
}
/**
 * Implements form submit for tripal_galaxy_admin_edit_file_form_submit form.
 */
function tripal_galaxy_admin_edit_file_form_submit($form, &$form_state) {
  $swfid = $form_state['values']['swfid'];
  $short_desc = trim($form_state['values']['short_desc']);
  $file_type = strtolower(trim($form_state['values']['file_type']));

  $num_updated = db_update('tripal_galaxy_site_files')
    ->fields(array(
      'short_desc' => $short_desc,
      'filetype' => $file_type,
    ))
    ->condition('swfid', $swfid)
    ->execute();

  if ($num_updated == 1) {
    drupal_set_message('Succesfully updated the file.');
    drupal_goto('admin/tripal/extension/galaxy/files');
  }
  else {
    drupal_set_message('A problem occured while trying to update the file.', 'error');
  }
}

/**
 * Deletes a site-wide file.
 *
 * @param $swfid
 *   The file ID
 */
function tripal_galaxy_admin_delete_file_form($form, &$form_state, $swfid) {
  $form = array();

  // Get the file.
  $fid = db_select('tripal_galaxy_site_files', 'tgsf')
    ->fields('tgsf', array('fid'))
    ->condition('swfid', $swfid)
    ->execute()
    ->fetchField();

  $form['swfid'] = array(
    '#type' => 'value',
    '#value' => $swfid,
  );
  $form['fid'] = array(
    '#type' => 'value',
    '#value' => $fid,
  );

  $file = file_load($fid);

  return confirm_form($form,
    t('Confirm removal of the file named "' . $file->filename . '"? '),
    'admin/tripal/extension/galaxy/files',
    t('Note if this file is used in a Galaxy workflow it will no longer show when a user views their submission.')
  );
}

/**
 * Implements submit hook for the tripal_galaxy_admin_delete_file_form.
 */
function tripal_galaxy_admin_delete_file_form_submit($form, &$form_state) {
  $fid = $form_state['values']['fid'];
  $swfid = $form_state['values']['swfid'];

  $success = db_delete('tripal_galaxy_site_files')
    ->condition('swfid', $swfid)
    ->execute();
  if (!$success) {
    drupal_set_message('The file could not be removed.', 'error');
    drupal_goto('admin/tripal/extension/galaxy/files');
    return;
  }
  $file = file_load($fid);
  file_usage_delete($file, 'tripal_galaxy');
  if (file_delete($file, TRUE)) {
    drupal_set_message('The file, "' . $file->filename . '", has been removed.');
  }
  else {
    drupal_set_message('The file could not be removed.', 'error');
  }
  drupal_goto('admin/tripal/extension/galaxy/files');
}
/**
 * Disables a file from being available for Galaxy workflow inputs.
 *
 * @param $swfid
 *   The file ID
 */
function tripal_galaxy_admin_disable_file($swfid) {
  $num_updated = db_update('tripal_galaxy_site_files')
    ->fields(array(
      'enabled' => '0'
    ))
    ->condition('swfid', $swfid)
    ->execute();
  if ($num_updated == 1) {
    drupal_set_message('Succesfully disabled the file. It is no longer available for use with Galaxy workflows.');
  }
  else {
    drupal_set_message('A problem occured while trying to enable the file.', 'error');
  }
  drupal_goto('admin/tripal/extension/galaxy/files');
}

/**
 * Enables a file from use with Galaxy workflow inputs.
 *
 * @param $swfid
 */
function tripal_galaxy_admin_enable_file($swfid) {
  $num_updated = db_update('tripal_galaxy_site_files')
    ->fields(array(
      'enabled' => '1'
    ))  ->condition('swfid', $swfid)
    ->execute();
  if ($num_updated == 1) {
    drupal_set_message('Succesfully enabled the file. It is now avilable for use with Galaxy workflows.');
  }
  else {
    drupal_set_message('A problem occured while trying to enable the file.', 'error');
  }
  drupal_goto('admin/tripal/extension/galaxy/files');
}