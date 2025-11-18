import React, { useState } from 'react';
import { View, Text, ScrollView, Button, Switch, StyleSheet } from 'react-native';

export default function QuestionnaireScreen({ navigation }) {
  const [goal, setGoal] = useState('strength');
  const [timeAvailable, setTimeAvailable] = useState(20);
  const [exercisePrefs, setExercisePrefs] = useState({
    bodyweight: true,
    equipment: false,
    lowImpact: false,
    highIntensity: true
  });
  const [conditions, setConditions] = useState({
    knee: false,
    back: false,
    shoulder: false,
    heart: false
  });
  const [difficulty, setDifficulty] = useState('medium');

  const exercisesDB = [
    { exercise: 'Push-ups', type:'bodyweight', difficulty:'medium', conditions:['knee','back'] },
    { exercise: 'Squats', type:'bodyweight', difficulty:'medium', conditions:['knee'] },
    { exercise: 'Glute Bridges', type:'bodyweight', difficulty:'easy', conditions:[] },
    { exercise: 'Jumping Jacks', type:'bodyweight', difficulty:'hard', conditions:['knee'] },
    { exercise: 'Bird Dogs', type:'bodyweight', difficulty:'easy', conditions:['back'] }
  ];

  const generateWorkout = () => {
    let workout = [];

    exercisesDB.forEach(e => {
      if(e.type === 'equipment' && !exercisePrefs.equipment) return;
      if(e.type === 'bodyweight' && !exercisePrefs.bodyweight) return;
      if(e.difficulty !== difficulty) return;

      const conflicts = Object.keys(conditions).filter(c => conditions[c]);
      if(e.conditions.some(cond => conflicts.includes(cond))) return;

      workout.push({ exercise: e.exercise, reps: 15, duration: 30 });
    });

    let totalDuration = 0;
    workout = workout.filter(e => {
      totalDuration += e.duration;
      return totalDuration <= timeAvailable * 60;
    });

    navigation.navigate('WorkoutBuilder', { workout });
  };

  return (
    <ScrollView style={styles.container}>
      <Text style={styles.heading}>Questionnaire</Text>

      <Text style={styles.subHeading}>Fitness Goal: {goal}</Text>
      <View style={styles.buttonRow}>
        <Button title="Strength" onPress={()=>setGoal('strength')} />
        <Button title="Cardio" onPress={()=>setGoal('cardio')} />
      </View>

      <Text style={styles.subHeading}>Time Available: {timeAvailable} min</Text>
      <View style={styles.buttonRow}>
        <Button title="10" onPress={()=>setTimeAvailable(10)} />
        <Button title="20" onPress={()=>setTimeAvailable(20)} />
        <Button title="30" onPress={()=>setTimeAvailable(30)} />
      </View>

      <Text style={styles.subHeading}>Exercise Preferences:</Text>
      {Object.keys(exercisePrefs).map(key => (
        <View key={key} style={styles.switchRow}>
          <Switch
            value={exercisePrefs[key]}
            onValueChange={(val)=>setExercisePrefs({...exercisePrefs, [key]: val})}
          />
          <Text style={styles.switchLabel}>{key}</Text>
        </View>
      ))}

      <Text style={styles.subHeading}>Health Conditions:</Text>
      {Object.keys(conditions).map(key => (
        <View key={key} style={styles.switchRow}>
          <Switch
            value={conditions[key]}
            onValueChange={(val)=>setConditions({...conditions, [key]: val})}
          />
          <Text style={styles.switchLabel}>{key}</Text>
        </View>
      ))}

      <Text style={styles.subHeading}>Difficulty: {difficulty}</Text>
      <View style={styles.buttonRow}>
        <Button title="Easy" onPress={()=>setDifficulty('easy')} />
        <Button title="Medium" onPress={()=>setDifficulty('medium')} />
        <Button title="Hard" onPress={()=>setDifficulty('hard')} />
      </View>

      <View style={{ marginTop:20 }}>
        <Button title="Generate Workout" onPress={generateWorkout} />
      </View>
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: { padding:20 },
  heading: { fontSize:22, fontWeight:'bold', marginBottom:10 },
  subHeading: { fontSize:16, fontWeight:'600', marginTop:10 },
  buttonRow: { flexDirection:'row', justifyContent:'space-around', marginVertical:5 },
  switchRow: { flexDirection:'row', alignItems:'center', marginBottom:5 },
  switchLabel: { marginLeft:8 }
});
