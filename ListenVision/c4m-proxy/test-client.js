#!/usr/bin/env node
/**
 * test-client.js
 * 
 * Simple test client to send UDP commands to the C4M proxy server
 * Use this to test the proxy without Q-SYS
 */

const dgram = require('dgram');

const PROXY_IP = process.env.PROXY_IP || 'localhost';
const PROXY_PORT = process.env.PROXY_PORT || 8888;

class TestClient {
  constructor(ip, port) {
    this.ip = ip;
    this.port = port;
    this.client = dgram.createSocket('udp4');
  }

  send(command) {
    const message = JSON.stringify(command);
    const buffer = Buffer.from(message);
    
    return new Promise((resolve, reject) => {
      this.client.send(buffer, 0, buffer.length, this.port, this.ip, (err) => {
        if (err) {
          reject(err);
        } else {
          console.log(`✓ Sent: ${message}`);
          resolve();
        }
      });
    });
  }

  async close() {
    this.client.close();
  }
}

// Test sequences
async function runTests() {
  const client = new TestClient(PROXY_IP, PROXY_PORT);
  
  console.log(`🧪 Testing C4M Proxy at ${PROXY_IP}:${PROXY_PORT}\n`);
  
  try {
    // Test 1: Clear all displays
    console.log('\n[Test 1] Clear all displays');
    await client.send({ cmd: 'clear', group: 0 });
    await sleep(1000);
    
    // Test 2: Test pattern (all groups)
    console.log('\n[Test 2] Test pattern (broadcast)');
    await client.send({ cmd: 'test', group: 0 });
    await sleep(3000);
    
    // Test 3: Set single segment text
    console.log('\n[Test 3] Set segment 0 text');
    await client.send({
      cmd: 'set_segment',
      group: 0,
      seg: 0,
      enabled: true,
      text: 'Hello C4M',
      color: 'FF0000',
      effect: 1, // scroll left
      align: 1   // center
    });
    await sleep(2000);
    
    // Test 4: Multiple segments
    console.log('\n[Test 4] Set all 4 segments');
    await client.send({
      cmd: 'set_segment',
      group: 0,
      seg: 0,
      enabled: true,
      text: 'Segment 0',
      color: 'FF0000'
    });
    await client.send({
      cmd: 'set_segment',
      group: 0,
      seg: 1,
      enabled: true,
      text: 'Segment 1',
      color: '00FF00'
    });
    await client.send({
      cmd: 'set_segment',
      group: 0,
      seg: 2,
      enabled: true,
      text: 'Segment 2',
      color: '0000FF'
    });
    await client.send({
      cmd: 'set_segment',
      group: 0,
      seg: 3,
      enabled: true,
      text: 'Segment 3',
      color: 'FFFF00'
    });
    await sleep(3000);
    
    // Test 5: Change layout
    console.log('\n[Test 5] Change layout preset');
    await client.send({
      cmd: 'set_layout',
      group: 0,
      preset: 2 // top/bottom split
    });
    await sleep(2000);
    
    // Test 6: Brightness control
    console.log('\n[Test 6] Set brightness');
    await client.send({
      cmd: 'set_brightness',
      group: 0,
      brightness: 128
    });
    await sleep(2000);
    
    // Test 7: Group-specific (Zone A only)
    console.log('\n[Test 7] Send to Group 1 only');
    await client.send({
      cmd: 'set_segment',
      group: 1, // Zone A
      seg: 0,
      enabled: true,
      text: 'Zone A Only',
      color: 'FF00FF'
    });
    await sleep(2000);
    
    // Test 8: Rotation
    console.log('\n[Test 8] Rotate display');
    await client.send({
      cmd: 'set_rotation',
      group: 0,
      rotation: 90
    });
    await sleep(2000);
    
    // Reset rotation
    await client.send({
      cmd: 'set_rotation',
      group: 0,
      rotation: 0
    });
    
    console.log('\n✅ All tests completed!');
    
  } catch (error) {
    console.error('\n❌ Test failed:', error.message);
  } finally {
    await client.close();
  }
}

// Stress test - send rapid updates
async function stressTest(duration = 10, rate = 10) {
  const client = new TestClient(PROXY_IP, PROXY_PORT);
  
  console.log(`🔥 Stress test: ${rate} Hz for ${duration} seconds\n`);
  
  const interval = 1000 / rate;
  const count = duration * rate;
  let sent = 0;
  
  const startTime = Date.now();
  
  for (let i = 0; i < count; i++) {
    const text = `Update ${i}`;
    const color = ['FF0000', '00FF00', '0000FF', 'FFFF00'][i % 4];
    
    await client.send({
      cmd: 'set_segment',
      group: 0,
      seg: 0,
      enabled: true,
      text: text,
      color: color
    });
    
    sent++;
    
    // Rate limiting
    const elapsed = Date.now() - startTime;
    const expected = (i + 1) * interval;
    const diff = expected - elapsed;
    
    if (diff > 0) {
      await sleep(diff);
    }
    
    // Progress
    if (i % rate === 0) {
      const actualRate = (sent / ((Date.now() - startTime) / 1000)).toFixed(1);
      console.log(`  ${i}/${count} updates sent (${actualRate} Hz)`);
    }
  }
  
  const totalTime = (Date.now() - startTime) / 1000;
  const actualRate = sent / totalTime;
  
  console.log(`\n✅ Stress test complete:`);
  console.log(`   Sent: ${sent} updates`);
  console.log(`   Time: ${totalTime.toFixed(2)}s`);
  console.log(`   Rate: ${actualRate.toFixed(2)} Hz`);
  
  await client.close();
}

// Helper
function sleep(ms) {
  return new Promise(resolve => setTimeout(resolve, ms));
}

// CLI
const args = process.argv.slice(2);
const command = args[0];

if (command === 'stress') {
  const duration = parseInt(args[1]) || 10;
  const rate = parseInt(args[2]) || 10;
  stressTest(duration, rate);
} else if (command === 'help') {
  console.log(`
C4M Proxy Test Client

Usage:
  node test-client.js              Run standard test sequence
  node test-client.js stress [duration] [rate]    
                                   Run stress test (default: 10s @ 10Hz)

Examples:
  node test-client.js
  node test-client.js stress 30 10
  
Environment:
  PROXY_IP=10.1.1.100 node test-client.js
  PROXY_PORT=8888 node test-client.js
  `);
} else {
  runTests();
}
