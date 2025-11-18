import React from 'react';
import { View, Text, StyleSheet } from 'react-native';

export default function WorkoutDetailScreen({ route }) {
  const { workout } = route.params;

  return (
    <View style={styles.container}>
      <Text style={styles.title}>{workout.name}</Text>
      <Text style={styles.detail}>Workout details will go here...</Text>
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, padding: 20 },
  title: { fontSize: 24, fontWeight: 'bold', marginBottom: 20 },
  detail: { fontSize: 16 },
});
