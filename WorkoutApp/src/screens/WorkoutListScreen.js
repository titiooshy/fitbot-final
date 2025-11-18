import React from 'react';
import { View, Text, FlatList, TouchableOpacity, StyleSheet } from 'react-native';

export default function WorkoutListScreen({ route, navigation }) {
  const { goal, frequency, level } = route.params || {};

  let workouts = [
    { id: '1', name: 'Full Body Blast', tags: ['Strength', 'Intermediate'] },
    { id: '2', name: 'Leg Day Strength', tags: ['Strength', 'Advanced'] },
    { id: '3', name: 'Core & Abs', tags: ['Endurance', 'Beginner'] },
    { id: '4', name: 'HIIT Fat Burner', tags: ['Weight Loss', 'Intermediate'] },
    { id: '5', name: 'Yoga Flow', tags: ['Flexibility', 'Beginner'] },
  ];

  // Filter workouts based on goal and level
  if (goal && level) {
    workouts = workouts.filter(w => w.tags.includes(goal) && w.tags.includes(level));
  }

  return (
    <View style={styles.container}>
      {workouts.length === 0 ? (
        <Text style={styles.noWorkoutText}>No workouts found for your preferences.</Text>
      ) : (
        <FlatList
          data={workouts}
          keyExtractor={item => item.id}
          renderItem={({ item }) => (
            <TouchableOpacity
              style={styles.workoutItem}
              onPress={() => navigation.navigate('WorkoutDetail', { workout: item })}
            >
              <Text style={styles.workoutText}>{item.name}</Text>
            </TouchableOpacity>
          )}
        />
      )}
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, padding: 20 },
  workoutItem: {
    padding: 20,
    marginBottom: 10,
    backgroundColor: '#eee',
    borderRadius: 10,
  },
  workoutText: { fontSize: 18 },
  noWorkoutText: { fontSize: 16, textAlign: 'center', marginTop: 50 },
});
