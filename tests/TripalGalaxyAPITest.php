<?php
namespace Tests;

use StatonLab\TripalTestSuite\DBTransaction;
use StatonLab\TripalTestSuite\TripalTestCase;

/**
 * Note: all test assumes that there is an instance of Galaxy running on the
 * network that is available via the URL http://galaxy/.
 * The docker-compose
 * setup file in this module provides that.
 */
class TripalGalaxyAPITest extends TripalTestCase {

  // Turn on transactions to rollback database updates after each test.
  // use DBTransaction;
  
  /**
   * Tests tripal_galaxy_add_galaxy().
   */
  public function testAddGalaxy() {
    $values = [
      'servername' => 'Local Galaxy',
      'url' => 'http://galaxy',
      'description' => 'A Local Galaxy server for testing.',
      'username' => 'admin@galaxy.org',
      'api_key' => 'HSNiugRFvgT574F43jZ7N9F3',
      'uid' => 1
    ];
    
    // Test required fields. Each should return FALSE
    $required_args = [
      'servername',
      'url',
      'username',
      'api_key',
      'uid'
    ];
    foreach ($required_args as $key) {
      $test_vals = $values;
      unset($test_vals[$key]);
      $galaxy = tripal_galaxy_add_galaxy($test_vals);
      $this->assertFalse($galaxy, 'tripal_galaxy_add_galaxy: the response ' . 'should be FALSE: missing parameter "' . $key . '"');
    }
    
    // Test proper addition of the galaxy server.
    $galaxy = tripal_galaxy_add_galaxy($values);
    $this->assertNotFalse($galaxy, 'tripal_galaxy_add_galaxy: the response should not be FALSE.');
    
    // Tests we get back an object.
    $this->assertTrue(is_object($galaxy), 'tripal_galaxy_add_galaxy: the response should be an object.');
    
    // Test that the object has a galaxy_kd
    $this->assertTrue(property_exists($galaxy, 'galaxy_id'), 'tripal_galaxy_add_galaxy: The galaxy object is missing the ' . 'galaxy_id property.');
    
    // Test adding a duplicate. The same galaxy_id should be returned.
    $galaxy_id = $galaxy->galaxy_id;
    $galaxy = tripal_galaxy_add_galaxy($values);
    $this->assertTrue($galaxy->galaxy_id == $galaxy_id, 'tripal_galaxy_add_galaxy: should return the same galaxy_id ' . 'for a duplicated entry.');
    
    return $galaxy;
  }

  /**
   * Tests tripal_galaxy_test_connection().
   * @depends testAddGalaxy
   */
  public function tripal_galaxy_get_galaxy($galaxy) {
    // Make sure we can find the Galaxy server we already added.
    $found = tripal_galaxy_get_galaxy($galaxy->galaxy_id);
    $this->assertTrue($galaxy->galaxy_id == $found->galaxy_id, 'tripal_galaxy_add_galaxy: should return the same galaxy_id ' . 'for a duplicated entry.');
  }

  /**
   * Tests tripal_galaxy_test_connection().
   * @depends testAddGalaxy
   */
  public function testConnection($galaxy) {
    
    // First check we can test if the galaxy_id is provided.
    $connection = tripal_galaxy_test_connection([
      'galaxy_id' => $galaxy->galaxy_id
    ]);
    $this->assertTrue($connection, 'tripal_galaxy_test_connection: connection using galaxy_id failed.');
    
    // Second check we can test connection using URL parts.
    $connection = tripal_galaxy_test_connection([
      'host' => 'galaxy',
      'port' => '80',
      'use_https' => FALSE
    ]);
    $this->assertTrue($connection, 'tripal_galaxy_test_connection: connection failed.');
  }

  /**
   * Setup the Galaxy server to have the workflow we need for testing.
   * This will
   * use blend4php directly and since we aren't testing that library we'll ust
   * load up the data here as a provider. @depends testAddGalaxy
   */
  public function loadWorkflow() {
  }
}
