#!/usr/bin/env node
/**
 * test-client.js - MQTT Test Client
 * 
 * Test the C4M proxy by publishing MQTT messages
 */

const mqtt = require('mqtt');

const BROKER = process.env.MQTT_BROKER || 'mqtt://localhost:1883';
const PREFIX = process.env.TOPIC_PREFIX || 'led';

class MQTTTestClient {
  constructor() {
    this.client = mqtt.connect(BROKER);
    this.ready = false;
    
    this.client.on('connect', () => {
      console.log(`✓ Connected to ${BROKER}`);
      this.ready = true;
      
      // Subscribe to status topics
      this.client.subscribe([
        `${PREFIX}/display/+/status`,
        `${PREFIX}/proxy/stats`,
        `${PREFIX}/proxy/status`
      ]);
    });
    
    this.client.on('message', (topic, message) => {
      const payload = JSON.parse(message.toString());
      console.log(`[${topic}]`, payload);
    });
  }

  async publish(topic, payload) {
    return new Promise((resolve, reject) => {
      this.client.publish(topic, JSON.stringify(payload), (err) => {
        if (err) {
          reject(err);
        } else {
          console.log(`✓ Published to ${topic}`);
          resolve();
        }
      });
    });
  }

  async close() {
    this.client.end();
  }
}

// Test sequences
async function runTests() {
  const client = new MQTTTestClient();
  
  // Wait for connection
  await new Promise(resolve => {
    const check = setInterval(() => {
      if (client.ready) {
        clearInterval(check);
        resolve();
      }
    }, 100);
  });
  
  console.log(`\n🧪 Testing C4M Proxy (MQTT)\n`);
  
  try {
    // Test 1: Clear all displays
    console.log('\n[Test 1] Clear all displays');
    await client.publish(`${PREFIX}/display/all/cmd/clear`, {});
    await sleep(1000);
    
    // Test 2: Test pattern on display ID 1
    console.log('\n[Test 2] Test pattern on display 1');
    await client.publish(`${PREFIX}/display/1/cmd/test`, {});
    await sleep(3000);
    
    // Test 3: Set segment text on specific display
    console.log('\n[Test 3] Set segment 0 on display 1');
    await client.publish(`${PREFIX}/display/1/cmd/segment`, {
      seg: 0,
      enabled: true,
      text: 'Hello MQTT!',
      color: 'FF0000',
      effect: 1,
      align: 1
    });
    await sleep(2000);
    
    // Test 4: Broadcast to all displays
    console.log('\n[Test 4] Broadcast to all displays');
    await client.publish(`${PREFIX}/display/all/cmd/segment`, {
      seg: 0,
      enabled: true,
      text: 'BROADCAST',
      color: '00FF00',
      effect: 0,
      align: 1
    });
    await sleep(2000);
    
    // Test 5: Send to group 1
    console.log('\n[Test 5] Send to group 1 only');
    await client.publish(`${PREFIX}/group/1/cmd/segment`, {
      seg: 0,
      enabled: true,
      text: 'Group 1 Only',
      color: '0000FF',
      effect: 0,
      align: 0
    });
    await sleep(2000);
    
    // Test 6: Multiple segments on display 2
    console.log('\n[Test 6] Set all segments on display 2');
    for (let i = 0; i < 4; i++) {
      await client.publish(`${PREFIX}/display/2/cmd/segment`, {
        seg: i,
        enabled: true,
        text: `Segment ${i}`,
        color: ['FF0000', '00FF00', '0000FF', 'FFFF00'][i],
        effect: 0,
        align: 1
      });
      await sleep(500);
    }
    await sleep(2000);
    
    // Test 7: Set layout
    console.log('\n[Test 7] Change layout on all displays');
    await client.publish(`${PREFIX}/display/all/cmd/layout`, {
      preset: 2
    });
    await sleep(2000);
    
    // Test 8: Brightness
    console.log('\n[Test 8] Set brightness');
    await client.publish(`${PREFIX}/display/all/cmd/brightness`, {
      brightness: 128
    });
    await sleep(2000);
    
    // Test 9: Request proxy stats
    console.log('\n[Test 9] Request proxy stats');
    await client.publish(`${PREFIX}/proxy/cmd/stats`, {});
    await sleep(1000);
    
    console.log('\n✅ All tests completed!');
    
  } catch (error) {
    console.error('\n❌ Test failed:', error.message);
  } finally {
    await sleep(2000); // Wait for status messages
    await client.close();
  }
}

// Stress test
async function stressTest(duration = 10, rate = 10, displayId = 1) {
  const client = new MQTTTestClient();
  
  await new Promise(resolve => {
    const check = setInterval(() => {
      if (client.ready) {
        clearInterval(check);
        resolve();
      }
    }, 100);
  });
  
  console.log(`🔥 Stress test: ${rate} Hz for ${duration} seconds on display ${displayId}\n`);
  
  const interval = 1000 / rate;
  const count = duration * rate;
  let sent = 0;
  
  const startTime = Date.now();
  
  for (let i = 0; i < count; i++) {
    const text = `Update ${i}`;
    const color = ['FF0000', '00FF00', '0000FF', 'FFFF00'][i % 4];
    
    await client.publish(`${PREFIX}/display/${displayId}/cmd/segment`, {
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
  
  await sleep(2000);
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
  const displayId = parseInt(args[3]) || 1;
  stressTest(duration, rate, displayId);
} else if (command === 'help') {
  console.log(`
C4M Proxy MQTT Test Client

Usage:
  node test-client.js              Run standard test sequence
  node test-client.js stress [duration] [rate] [displayId]
                                   Run stress test (default: 10s @ 10Hz on display 1)

Examples:
  node test-client.js
  node test-client.js stress 30 10 42
  
Environment:
  MQTT_BROKER=mqtt://10.1.1.100:1883 node test-client.js
  TOPIC_PREFIX=led node test-client.js

MQTT Topics:
  Commands:
    led/display/1/cmd/segment      - Set segment on display 1
    led/display/all/cmd/clear      - Clear all displays
    led/group/1/cmd/test           - Test pattern on group 1
  
  Status (subscribe):
    led/display/+/status           - Device status updates
    led/proxy/stats                - Proxy statistics
  `);
} else {
  runTests();
}
