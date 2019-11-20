<?php

/**
 * @file
 * Custom views handler definition.
 */

// phpcs:disable Drupal.NamingConventions.ValidClassName.NoUnderscores

// phpcs:disable Drupal.NamingConventions.ValidClassName.StartWithCaptial

/**
 * Custom handler class.
 *
 * @ingroup views_field_handlers
 */
class views_handler_view_delete_field extends views_handler_field {

  /**
   * Perform any database or cache data retrieval here.
   */
  public function query() {
  }

  // phpcs:disable Drupal.NamingConventions.ValidFunctionName.ScopeNotCamelCaps

  /**
   * Modify any end user views settings here.
   *
   * Debug $options to view the field settings you can change.
   */
  public function option_definition() {
    $options = parent::option_definition();
    return $options;
  }

  // phpcs:disable Generic.CodeAnalysis.UselessOverridingMethod.Found



  /**
   * Make changes to the field settings form seen by the end user.
   */
  public function options_form(&$form, &$form_state) {
    parent::options_form($form, $form_state);
  }

  // phpcs:enable Generic.CodeAnalysis.UselessOverridingMethod.Found

  // phpcs:enable Drupal.NamingConventions.ValidFunctionName.ScopeNotCamelCaps

  /**
   * Render callback handler.
   *
   * Return the markup that will appear in the rendered field.
   */
  public function render($values) {

    // Check status of the run.
    if ($values->tripal_galaxy_workflow_submission_status == 'Completed' or
        $values->tripal_galaxy_workflow_submission_status == 'Error') {
      $submission = tripal_galaxy_get_submission($values->sid);
      $path = 'user/' . $submission->uid . '/galaxy-jobs/' . $values->sid;
      $this->options['alter']['make_link'] = TRUE;
      $this->options['alter']['path'] = $path;
      return 'Delete';
    }
    $this->last_render_text = '';

    return NULL;
  }

}

// phpcs:enable Drupal.NamingConventions.ValidClassName.StartWithCaptial
// phpcs:enable Drupal.NamingConventions.ValidClassName.NoUnderscores