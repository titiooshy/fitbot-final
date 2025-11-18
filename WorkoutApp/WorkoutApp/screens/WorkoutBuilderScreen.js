import React from 'react';
import { View, Text, FlatList, Button, StyleSheet } from 'react-native';

export default function WorkoutBuilderScreen({ route }) {
  const { workout } = route.params;

  return (
    <View style={styles.container}>
      <Text style={styles.heading}>Your Workout</Text>
      <FlatList
        data={workout}
        keyExtractor={(item, index) => index.toString()}
        renderItem={({ item }) => (
          <Text style={styles.item}>{item.exercise} - {item.reps} reps / {item.duration}s</Text>
        )}
      />
      <View style={{ marginTop:20 }}>
        <Button title="Send to Device (BLE)" onPress={()=>alert("BLE sending not implemented yet")} />
      </View>
    </View>
  );
}

const styles = StyleSheet.create({
  container: { padding:20, flex:1 },
  heading: { fontSize:20, fontWeight:'bold', marginBottom:10 },
  item: { fontSize:16, marginVertical:4 }
});
