<?php
namespace Tests;

use StatonLab\TripalTestSuite\DBTransaction;
use StatonLab\TripalTestSuite\TripalTestCase;

/**
 * Note: all test assumes that there is an instance of Galaxy running on
 * the network that is available via the URL http://galaxy/.  The
 * docker-compose setup file in this module provides that.
 */

class TripalGalaxyAPITest extends TripalTestCase {
  // Uncomment to auto start and rollback db transactions per test method.
  use DBTransaction;
  
  /**
   * Adds a new Galaxy server.
   */
  public function testAddGalaxy() {
    $values = [
      'servername' => 'Local Galaxy',
      'url' => 'http://galaxy',
      'description' => 'A Local Galaxy server for testing.',
      'username' => 'admin@galaxy.org',
      'api_key' => 'HSNiugRFvgT574F43jZ7N9F3',
      'uid' => 1,
    ];
    
    // Test required fields. Each should return FALSE
    $required_args = ['servername', 'url', 'username', 'api_key', 'uid'];
    foreach ($required_args as $key) {
      $test_vals = $values;
      unset($test_vals[$key]);
      $galaxy = tripal_galaxy_add_galaxy($values);
      $this->assertFalse($galaxy,
        'tripal_galaxy_add_galaxy: the response ' .
        'should be FALSE: missing ' . $key);      
    }
    
    
    // Test proper addition of the galaxy server.
    $galaxy = tripal_galaxy_add_galaxy($values);
    $this->assertNotFalse($galaxy, 
      'tripal_galaxy_add_galaxy: the response should not be FALSE.');
    
    // Tests we get back an object.
    $this->assertTrue(is_object($galaxy), 
      'tripal_galaxy_add_galaxy: the response should be an object.');
    
    // Test that the object has a galaxy_kd
    $this->assertTrue(property_exists($galaxy, 'galaxy_id'), 
      'tripal_galaxy_add_galaxy: The galaxy object is missing the ' . 
      'galaxy_id property.');
        
  }
}
