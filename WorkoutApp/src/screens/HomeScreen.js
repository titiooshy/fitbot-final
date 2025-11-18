import React from 'react';
import { View, Text, Button, StyleSheet } from 'react-native';

export default function HomeScreen({ navigation }) {
  return (
    <View style={styles.container}>
      <Text style={styles.title}>Welcome to WorkoutApp</Text>
      <Text style={styles.subtitle}>
        Answer a few questions to get personalized workout suggestions.
      </Text>

      <View style={styles.buttonContainer}>
        <Button
          title="Start Questionnaire"
          onPress={() => navigation.navigate('Questionnaire')}
        />
      </View>

      <View style={styles.buttonContainer}>
        <Button
          title="View Profile"
          onPress={() => navigation.navigate('Profile')}
        />
      </View>
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, justifyContent: 'center', alignItems: 'center', padding: 20 },
  title: { fontSize: 28, fontWeight: 'bold', marginBottom: 20, textAlign: 'center' },
  subtitle: { fontSize: 16, textAlign: 'center', marginBottom: 40 },
  buttonContainer: { marginVertical: 10, width: '80%' },
});
