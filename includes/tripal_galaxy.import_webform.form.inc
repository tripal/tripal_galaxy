<?php

/**
 * @param $form
 * @param $form_state
 * @param $workflow_and_node_ids
 *
 * @return mixed
 *
 * form builder
 */
function galaxy_webform_import_form($form, &$form_state, $workflow_and_node_ids) {
  $form['workflow_and_node_ids'] = [
    '#type' => 'value',
    '#value' => $workflow_and_node_ids];
  $form['upload'] = [
    '#type' => 'managed_file',
    '#upload_validators' => [
      'file_validate_extensions' => [
        'json']]];
  $form['submit'] = [
    '#type' => 'submit',
    '#value' => 'Submit'];

  return $form;
}

/**
 *
 * @param
 *        $form
 * @param $form_state form
 *        submit callback.
 */
function galaxy_webform_import_form_submit($form, &$form_state) {
  // $workflow_id =
  // $form_state['values']['workflow_and_node_ids']['workflow_id'];
  $nid = $form_state['values']['workflow_and_node_ids']['nid'];
  $workflow_id = $form_state['values']['workflow_and_node_ids']['workflow_id'];

  // decode webform josn file to webform components.
  $fid = $form_state['values']['upload'];
  $uri = file_load($fid)->uri;
  $webform_json = file_get_contents($uri);
  $webform = json_decode($webform_json, TRUE);

  // delete original webform components.
  db_delete('webform_component')->condition('nid', $nid)->execute();
  // insert new components from imported webform.
  // $webform->components[1]->extra->workflow_id = $workflow_id;
  foreach ($webform['components'] as $component) {
    // keep original nid
    $component['nid'] = $nid;
    // keep original workflow_id. otherwise the workflow may not be found on the
    // galaxy server.
    if (isset($component['extra']['workflow_id'])) {
      $component['extra']['workflow_id'] = $workflow_id;
    }
    $component['extra'] = serialize($component['extra']);
    unset($component['page_num']);
    db_insert('webform_component')->fields($component)->execute();
  }
  drupal_set_message('The Galaxy webform has been successfully replaced!');
}
