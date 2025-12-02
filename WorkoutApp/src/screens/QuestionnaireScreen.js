import React, { useState } from 'react';
import { View, Text, Button, StyleSheet, ScrollView } from 'react-native';
import { Picker } from '@react-native-picker/picker'; // Make sure to install: npm install @react-native-picker/picker

export default function QuestionnaireScreen({ navigation }) {
  const [goal, setGoal] = useState('Strength');
  const [frequency, setFrequency] = useState('3-4');
  const [level, setLevel] = useState('Beginner');

  const handleSubmit = () => {
    navigation.navigate('Workouts', { goal, frequency, level });
  };

  return (
    <ScrollView contentContainerStyle={styles.container}>
      <Text style={styles.title}>Tell us about your fitness</Text>

      <Text style={styles.label}>Goal:</Text>
      <Picker selectedValue={goal} onValueChange={(v) => setGoal(v)} style={styles.picker}>
        <Picker.Item label="Strength" value="Strength" />
        <Picker.Item label="Endurance" value="Endurance" />
        <Picker.Item label="Weight Loss" value="Weight Loss" />
        <Picker.Item label="Flexibility" value="Flexibility" />
      </Picker>

      <Text style={styles.label}>Frequency:</Text>
      <Picker selectedValue={frequency} onValueChange={(v) => setFrequency(v)} style={styles.picker}>
        <Picker.Item label="1-2 days/week" value="1-2" />
        <Picker.Item label="3-4 days/week" value="3-4" />
        <Picker.Item label="5-7 days/week" value="5-7" />
      </Picker>

      <Text style={styles.label}>Experience Level:</Text>
      <Picker selectedValue={level} onValueChange={(v) => setLevel(v)} style={styles.picker}>
        <Picker.Item label="Beginner" value="Beginner" />
        <Picker.Item label="Intermediate" value="Intermediate" />
        <Picker.Item label="Advanced" value="Advanced" />
        <Picker.Item label="Expert" value="Expert" />
      </Picker>

      <View style={styles.buttonContainer}>
        <Button title="Get Suggestions" onPress={handleSubmit} />
      </View>
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: {
    padding: 20,
  },
  title: { fontSize: 24, fontWeight: 'bold', marginBottom: 20, textAlign: 'center' },
  label: { fontSize: 18, marginTop: 20 },
  picker: { marginTop: 10 },
  buttonContainer: { marginTop: 40 },
});
